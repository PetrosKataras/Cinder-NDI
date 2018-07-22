#include "CinderNDIReceiver.h"
#include <cstdio>
#define CI_MIN_LOG_LEVEL 2
#include "cinder/Log.h"
#include "cinder/Surface.h"
#include "cinder/gl/Sync.h"

CinderNDIReceiver::CinderNDIReceiver( const Description dscr )
{
	if( ! NDIlib_initialize() ) {
		throw std::runtime_error( "Cannot run NDI on this machine. Probably unsupported CPU." );
	}
	NDIlib_recv_create_v3_t recvDscr;
	recvDscr.source_to_connect_to = dscr.source != nullptr ? *(dscr.source) : NDISource();
	recvDscr.color_format = (NDIlib_recv_color_format_e)dscr.mColorFormat;
	recvDscr.bandwidth = (NDIlib_recv_bandwidth_e)dscr.mBandwidth;
	recvDscr.allow_video_fields = dscr.mAllowVideoFields;
	recvDscr.p_ndi_name = dscr.mName != "" ? dscr.mName.c_str() : nullptr;
	mNDIReceiver = NDIlib_recv_create_v3( &recvDscr );	
	if( ! mNDIReceiver ) {
		throw std::runtime_error( "Cannot create NDI Receiver. NDIlib_recv_create_v3 returned nullptr" );
	}	
	mVideoFramesBuffer = std::make_unique<VideoFramesBuffer>( 5 );
	auto ctx = ci::gl::Context::create( ci::gl::context() );
	mVideoRecvThread = std::make_unique<std::thread>( std::bind( &CinderNDIReceiver::videoRecvThread, this, ctx ) );
}

CinderNDIReceiver::~CinderNDIReceiver()
{
	mExitVideoThread = true;
	mVideoFramesBuffer->cancel();
	mVideoRecvThread->join();

	if( mNDIReceiver ) {
		NDIlib_recv_destroy( mNDIReceiver );
		mNDIReceiver = nullptr;
	}
	NDIlib_destroy();
}

void CinderNDIReceiver::videoRecvThread( ci::gl::ContextRef ctx )
{
	ctx->makeCurrent();
	while( ! mExitVideoThread ) {
		receiveVideo();
	}
}

void CinderNDIReceiver::connect( const NDISource& source )
{
	NDIlib_recv_connect( mNDIReceiver, &source );
}

void CinderNDIReceiver::disconnect()
{
	NDIlib_recv_connect( mNDIReceiver, nullptr );
}

ci::gl::TextureRef CinderNDIReceiver::getVideoTexture()
{
	if( mVideoFramesBuffer->isNotEmpty() ) {
		mVideoFramesBuffer->popBack( &mVideoTexture );
	}
	return mVideoTexture;
}

void CinderNDIReceiver::receiveVideo()
{
	NDIlib_video_frame_v2_t videoFrame;
	// NDIlib_recv_capture_v2 should be safe to call at the same time from multiple threads according to the SDK.
	// e.g To capture video and audio at the same time from separate threads for example.
	// Wait max .5 sec for a new frame to arrive.
	switch( NDIlib_recv_capture_v2( mNDIReceiver, &videoFrame, nullptr, nullptr, 500 ) ) { 
		case NDIlib_frame_type_none:
		{
			CI_LOG_V( "No data available...." ); 
			break;
		}
		case NDIlib_frame_type_video:
		{
			CI_LOG_V( "Received video frame with resolution : ( " << videoFrame.xres << ", " << videoFrame.yres << " ) " );
			auto surface = ci::Surface::create( videoFrame.p_data, videoFrame.xres, videoFrame.yres, videoFrame.line_stride_in_bytes, ci::SurfaceChannelOrder::RGBA );
			auto tex = ci::gl::Texture::create( *surface );
			auto fence = ci::gl::Sync::create();
			fence->clientWaitSync();
			mVideoFramesBuffer->pushFront( tex );
			NDIlib_recv_free_video_v2( mNDIReceiver, &videoFrame );
			break;
		}
	}
}
