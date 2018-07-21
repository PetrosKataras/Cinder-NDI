#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "CinderNDISender.h"
#include "cinder/qtime/QuickTimeGl.h"
#include "AsyncSurfaceReader.h"
using namespace ci;
using namespace ci::app;

using AsyncSurfaceReaderPtr = std::unique_ptr<AsyncSurfaceReader>;
class BasicSenderApp : public App {
  public:
	  BasicSenderApp() {}
	
	void setup() override;
	void update() override;
	void draw() override;

  private:
	void loadMovieFile( const fs::path &moviePath );
  private:
	qtime::MovieGlRef		mMovie;
	gl::TextureRef			mFrameTexture;
	ci::SurfaceRef 			mSurface;
	AsyncSurfaceReaderPtr 	mAsyncSurfaceReader;
	CinderNDISenderPtr		mCinderNDISender;
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

	fs::path moviePath = getOpenFilePath();
	if( ! moviePath.empty() ) {
		loadMovieFile( moviePath );
	}

	mAsyncSurfaceReader = std::make_unique<AsyncSurfaceReader>( getWindowWidth(), getWindowHeight() );
	CinderNDISender::Description senderDscr;
	senderDscr.mName = "Cinder_NDI_Sender";
	senderDscr.mClockVideo = true;
	mCinderNDISender = std::make_unique<CinderNDISender>( senderDscr );
}

void BasicSenderApp::update()
{
	getWindow()->setTitle( "CinderNDI-Sender - " + std::to_string( (int) getAverageFps() ) + " FPS" );

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
	if( mSurface && ! mFrameTexture )
		mFrameTexture = ci::gl::Texture::create( *mSurface );

	mCinderNDISender->send( mSurface.get() );
}

void BasicSenderApp::draw()
{
	gl::clear( ColorA::black() );

	if( mSurface ) {
		//mFrameTexture->setTopDown( true );
		mFrameTexture->update( *mSurface );
		//mFrameTexture->setTopDown( false );
		Rectf centeredRect = Rectf( mFrameTexture->getBounds() ).getCenteredFit( getWindowBounds(), true );
		gl::draw( mFrameTexture, centeredRect );
	}
}

// This line tells Cinder to actually create and run the application.
CINDER_APP( BasicSenderApp, RendererGl, prepareSettings )
