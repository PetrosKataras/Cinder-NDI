#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "CinderNDIReceiver.h"
#include "CinderNDIFinder.h"

using namespace ci;
using namespace ci::app;

class BasicReceiverApp : public App {
public:
	void setup() final;
	void update() final;
	void draw() final;
	void cleanup() final;
private:
	void sourceAdded( const NDISource& source );
	void sourceRemoved( const std::string sourceName );
private:
	CinderNDIFinderPtr mCinderNDIFinder;
	CinderNDIReceiverPtr mCinderNDIReceiver;
	ci::signals::Connection mNDISourceAdded;
	ci::signals::Connection mNDISourceRemoved;
};

void prepareSettings( BasicReceiverApp::Settings* settings )
{
}

void BasicReceiverApp::cleanup()
{
	if( mCinderNDIFinder ) {
		mNDISourceAdded.disconnect();	
		mNDISourceRemoved.disconnect();
	}
}

void BasicReceiverApp::sourceAdded( const NDISource& source )
{
	std::cout << "NDI source added: " << source.p_ndi_name <<std::endl;
	// Create the NDI receiver for this source
	CinderNDIReceiver::Description recvDscr;
	recvDscr.source = &source;
	if( ! mCinderNDIReceiver )
		mCinderNDIReceiver = std::make_unique<CinderNDIReceiver>( recvDscr );
	else
		mCinderNDIReceiver->connect( source );
}

void BasicReceiverApp::sourceRemoved( std::string sourceName )
{
	std::cout << "NDI source removed: " << sourceName <<std::endl;
}

void BasicReceiverApp::setup()
{
	// Create the NDI finder
	CinderNDIFinder::Description finderDscr;
	mCinderNDIFinder = std::make_unique<CinderNDIFinder>( finderDscr );
	
	mNDISourceAdded = mCinderNDIFinder->getSignalNDISourceAdded().connect(
		std::bind( &BasicReceiverApp::sourceAdded, this, std::placeholders::_1 )
	);
	mNDISourceRemoved = mCinderNDIFinder->getSignalNDISourceRemoved().connect(
		std::bind( &BasicReceiverApp::sourceRemoved, this, std::placeholders::_1 )
	);
}

void BasicReceiverApp::update()
{
	getWindow()->setTitle( "CinderNDI-Receiver - " + std::to_string( (int) getAverageFps() ) + " FPS" );
	if( mCinderNDIReceiver )
		mCinderNDIReceiver->receive();
}

void BasicReceiverApp::draw()
{
	gl::clear( ColorA::black() );
}

// This line tells Cinder to actually create and run the application.
CINDER_APP( BasicReceiverApp, RendererGl, prepareSettings )
