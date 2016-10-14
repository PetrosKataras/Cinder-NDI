#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "CinderNDISender.h"
#include "cinder/qtime/QuickTimeGl.h"
using namespace ci;
using namespace ci::app;

class BasicSenderApp : public App {
  public:
	  BasicSenderApp() : mSender( "test-cinder-video" ){}
	
	void setup() override;
	void update() override;
	void draw() override;

  private:
	void loadMovieFile( const fs::path &moviePath );
  private:
	CinderNDISender mSender;
	qtime::MovieGlRef		mMovie;
	gl::TextureRef			mFrameTexture;
	ci::SurfaceRef 			mSurface;
	uint8_t mIndexNew, mIndexOld;
	gl::FboRef mFbo[2];
	gl::PboRef mPbo[2];
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

	mIndexNew = 0;
	mIndexOld = 1;

	mFbo[0] = gl::Fbo::create( getWindowWidth(), getWindowHeight(), false );
	mFbo[1] = gl::Fbo::create( getWindowWidth(), getWindowHeight(), false );

	mPbo[0] = gl::Pbo::create( GL_PIXEL_PACK_BUFFER, getWindowWidth() * getWindowHeight() * 4, 0, GL_STREAM_READ );
	mPbo[1] = gl::Pbo::create( GL_PIXEL_PACK_BUFFER, getWindowWidth() * getWindowHeight() * 4, 0, GL_STREAM_READ );

	mSender.setup();
}

void BasicSenderApp::update()
{
	getWindow()->setTitle( "CinderNDI-Sender - " + std::to_string( (int) getAverageFps() ) + " FPS" );

	if( ! mSurface && mMovie->getTexture() ) {
		//mSurface = Surface::create( mMovie->getTexture()->createSource() );
		mSurface = Surface::create( getWindowWidth(), getWindowHeight(), GL_RGBA );
		mFrameTexture = ci::gl::Texture::create( *mSurface );
	}

	if( mSurface )
	{
		gl::ScopedFramebuffer sFbo( mFbo[mIndexOld] );
		gl::ScopedBuffer scopedPbo( mPbo[mIndexNew] );
		
		gl::readBuffer( GL_COLOR_ATTACHMENT0 );
		gl::readPixels( 0, 0, mFbo[mIndexOld]->getWidth(), mFbo[mIndexOld]->getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, 0 );
		mPbo[mIndexOld]->getBufferSubData( 0, mFbo[mIndexOld]->getWidth() * mFbo[mIndexOld]->getHeight() * 4, mSurface->getData() ); 
	}

	{
		gl::ScopedFramebuffer sFbo( mFbo[mIndexNew] );
		gl::ScopedViewport sVp( 0, 0, mFbo[mIndexNew]->getWidth(), mFbo[mIndexNew]->getHeight() );
		gl::clear( ColorA::black() );
		if( mMovie && mMovie->getTexture() ) {
			Rectf centeredRect = Rectf( mMovie->getTexture()->getBounds() ).getCenteredFit( getWindowBounds(), true );
			gl::draw( mMovie->getTexture(), centeredRect );
		}
	}
	mSender.sendSurface( mSurface );
}

void BasicSenderApp::draw()
{
	gl::clear( ColorA::black() );

	if( mSurface ) {
		mFrameTexture->setTopDown( true );
		mFrameTexture->update( *mSurface );
		mFrameTexture->setTopDown( false );
		Rectf centeredRect = Rectf( mFrameTexture->getBounds() ).getCenteredFit( getWindowBounds(), true );
		gl::draw( mFrameTexture, centeredRect );
	}
	std::swap( mIndexNew, mIndexOld );
}

// This line tells Cinder to actually create and run the application.
CINDER_APP( BasicSenderApp, RendererGl, prepareSettings )
