#include "CinderNDIReceiver.h"
#include <cstdio>
#include "cinder/Log.h"
#include "cinder/Surface.h"

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
}

CinderNDIReceiver::~CinderNDIReceiver()
{
	if( mNDIReceiver ) {
		NDIlib_recv_destroy( mNDIReceiver );
		mNDIReceiver = nullptr;
	}
	NDIlib_destroy();
}

void CinderNDIReceiver::connect( const NDISource& source )
{
	NDIlib_recv_connect( mNDIReceiver, &source );
}

void CinderNDIReceiver::disconnect()
{
	NDIlib_recv_connect( mNDIReceiver, nullptr );
}

void CinderNDIReceiver::receive()
{
	NDIlib_video_frame_v2_t videoFrame;
	switch( NDIlib_recv_capture_v2( mNDIReceiver, &videoFrame, nullptr, nullptr, 0 ) ) {
		case NDIlib_frame_type_none:
		{
			CI_LOG_V( "No data available...." ); 
			break;
		}
		case NDIlib_frame_type_video:
		{
			CI_LOG_V( "Received video frame with resolution : ( " << videoFrame.xres << ", " << videoFrame.yres << " ) " );
			NDIlib_recv_free_video_v2( mNDIReceiver, &videoFrame );
			break;
		}
	}
}
