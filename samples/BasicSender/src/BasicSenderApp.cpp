#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "CinderNDISender.h"
#include "cinder/qtime/QuickTimeGl.h"
#include "AsyncSurfaceReader.h"
#include "cinder/audio/audio.h"
using namespace ci;
using namespace ci::app;

using AsyncSurfaceReaderPtr = std::unique_ptr<AsyncSurfaceReader>;
class BasicSenderApp : public App {
  public:
	  BasicSenderApp() {}
	
	void setup() override;
	void update() override;
	void draw() override;
	void keyDown( KeyEvent event ) final;
	void fileDrop( FileDropEvent event ) final;
	void resize() final;

  private:
	void loadMovieFile( const fs::path &moviePath );
  private:
	qtime::MovieGlRef		mMovie;
	gl::TextureRef			mFrameTexture;
	ci::SurfaceRef 			mSurface;
	AsyncSurfaceReaderPtr 	mAsyncSurfaceReader;
	CinderNDISenderPtr		mCinderNDISender;
	audio::BufferPlayerNodeRef		mBufferPlayerNode;
	audio::GainNodeRef				mGain;
	audio::SourceFileRef 			mAudioSourceFile;
	audio::Buffer					mAudioFrame;
	uint32_t						mAudioFrameOffset;
};

void prepareSettings( BasicSenderApp::Settings* settings )
{
}

void BasicSenderApp::loadMovieFile( const fs::path &moviePath )
{
	try {
		// load up the movie, set it to loop, and begin playing
		mMovie = qtime::MovieGl::create( moviePath );
		mMovie->setLoop();
		mMovie->play();
		console() << "Playing: " << mMovie->isPlaying() << std::endl;
	}
	catch( ci::Exception &exc ) {
		console() << "Exception caught trying to load the movie from path: " << moviePath << ", what: " << exc.what() << std::endl;
		mMovie.reset();
	}

	mFrameTexture.reset();
}

void BasicSenderApp::setup()
{

	auto ctx = audio::Context::master();

	mBufferPlayerNode = ctx->makeNode( new audio::BufferPlayerNode() );

	// add a Gain to reduce the volume
	mGain = ctx->makeNode( new audio::GainNode( 0.0f ) );

	// connect and enable the Context
	mBufferPlayerNode >> mGain >> ctx->getOutput();
	ctx->enable();

	fs::path moviePath = getOpenFilePath();
	if( ! moviePath.empty() ) {
		loadMovieFile( moviePath );
	}

	mAsyncSurfaceReader = std::make_unique<AsyncSurfaceReader>( getWindowWidth(), getWindowHeight() );
	CinderNDISender::Description senderDscr;
	senderDscr.mName = "Cinder_NDI_Sender";
	senderDscr.mClockVideo = true;
	senderDscr.mClockAudio = true;
	mCinderNDISender = std::make_unique<CinderNDISender>( senderDscr );
}

void BasicSenderApp::resize()
{
	mAsyncSurfaceReader.reset( new AsyncSurfaceReader( getWindowWidth(), getWindowHeight() ) );
}

void BasicSenderApp::update()
{
	getWindow()->setTitle( "CinderNDI-Sender - " + std::to_string( (int) getAverageFps() ) + " FPS" );
	// Send the audio
	if( mBufferPlayerNode && mAudioSourceFile && mBufferPlayerNode->isEnabled() ) {
		int samplesPerFrame = ci::audio::Context::master()->getSampleRate() / mCinderNDISender->getFps();
		int totalFrameNum = mBufferPlayerNode->getNumFrames();
		if( mAudioFrameOffset < totalFrameNum ) {
			int distToEof = totalFrameNum - mAudioFrameOffset;
			if( distToEof < samplesPerFrame )
				samplesPerFrame = distToEof;
			CinderNDISender::AudioFrameParams audioParams;
			audioParams.mSampleRate = ci::audio::Context::master()->getSampleRate();
			mAudioFrame = audio::Buffer( samplesPerFrame, mBufferPlayerNode->getNumChannels() );
			// Copy one frame of audio containing samplesPerFrame and send it to the network.
			mAudioFrame.copyOffset( *mBufferPlayerNode->getBuffer().get(), samplesPerFrame, 0, mAudioFrameOffset );
			mCinderNDISender->sendAudio( &mAudioFrame, &audioParams );
			// Advance our audio frame for the next round.
			mAudioFrameOffset += samplesPerFrame;
		}
	}
	// Send the video
	{
		mAsyncSurfaceReader->bind();
		gl::ScopedViewport sVp( 0, 0, mAsyncSurfaceReader->getWidth(), mAsyncSurfaceReader->getHeight() );
		gl::clear( ColorA::black() );
		if( mMovie && mMovie->getTexture() ) {
			Rectf centeredRect = Rectf( mMovie->getTexture()->getBounds() ).getCenteredFit( getWindowBounds(), true );
			gl::draw( mMovie->getTexture(), centeredRect );
		}
		mAsyncSurfaceReader->unbind();
	}
	mSurface = mAsyncSurfaceReader->readPixels();
	mCinderNDISender->sendSurface( mSurface.get() );
	// Create our preview texture
	if( mSurface ) {
		if( ! mFrameTexture || ( mFrameTexture->getSize() != mSurface->getSize() ) ) {
			mFrameTexture = ci::gl::Texture::create( *mSurface );
		}
	}

}

void BasicSenderApp::draw()
{
	gl::clear( ColorA::black() );
	// Preview our NDI output.
	if( mSurface ) {
		mFrameTexture->update( *mSurface );
		Rectf centeredRect = Rectf( mFrameTexture->getBounds() ).getCenteredFit( getWindowBounds(), true );
		gl::draw( mFrameTexture, centeredRect );
	}
	/*
	if( mMovie ) {
		Rectf centeredRect = Rectf( mMovie->getTexture()->getBounds() ).getCenteredFit( getWindowBounds(), true );
		gl::draw( mMovie->getTexture(), centeredRect );
	}
	*/
}

void BasicSenderApp::fileDrop( FileDropEvent event )
{
	fs::path filePath = event.getFile( 0 );
	mAudioSourceFile = audio::load( loadFile( filePath ) );
	mBufferPlayerNode->loadBuffer( mAudioSourceFile );
	mBufferPlayerNode->enable();
	mBufferPlayerNode->seek( 0.0f );
	mAudioFrameOffset = 0;
}

void BasicSenderApp::keyDown( KeyEvent event )
{
	if( event.getCode() == KeyEvent::KEY_SPACE ) {
		if( mBufferPlayerNode->isEnabled() )
			mBufferPlayerNode->stop();
		else {
			mBufferPlayerNode->start();
		}
	}
}

// This line tells Cinder to actually create and run the application.
CINDER_APP( BasicSenderApp, RendererGl, prepareSettings )
