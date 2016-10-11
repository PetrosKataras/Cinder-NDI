#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "CinderNDIReceiver.h"

using namespace ci;
using namespace ci::app;

class BasicReceiverApp : public App {
  public:
	
	void setup() override;
	void update() override;
	void draw() override;

  private:
	CinderNDIReceiver mReceiver;
};

void prepareSettings( BasicReceiverApp::Settings* settings )
{
}

void BasicReceiverApp::setup()
{
	mReceiver.setup();
}

void BasicReceiverApp::update()
{
	mReceiver.update();
	getWindow()->setTitle( "CinderNDI-Receiver - " + std::to_string( (int) getAverageFps() ) + " FPS" );

}

void BasicReceiverApp::draw()
{
	gl::clear( ColorA::black() );
	auto tex = mReceiver.getVideoTexture();
	if( tex ) {
		Rectf centeredRect = Rectf( tex->getBounds() ).getCenteredFit( getWindowBounds(), true );
		gl::draw( tex, centeredRect );
	}
}

// This line tells Cinder to actually create and run the application.
CINDER_APP( BasicReceiverApp, RendererGl, prepareSettings )
