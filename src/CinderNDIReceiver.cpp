#include "CinderNDIReceiver.h"
#include <cstdio>

#include "cinder/Log.h"
#include "cinder/Surface.h"
#include <Processing.NDI.Recv.h>

CinderNDIReceiver::CinderNDIReceiver()
{
	if( ! NDIlib_is_supported_CPU() ) {
		CI_LOG_E( "Failed to initialize NDI because of unsupported CPU!" );
	}

	if( ! NDIlib_initialize() ) {
		CI_LOG_E( "Failed to initialize NDI!" );
	}

	mNdiInitialized = true;
}

CinderNDIReceiver::~CinderNDIReceiver()
{
	if( mNdiFinder ) {
		NDIlib_find_destroy( mNdiFinder );	
	}
	if( mNdiReceiver ) {
		NDIlib_recv_destroy( mNdiReceiver );
	}

	NDIlib_destroy();
	mNdiInitialized = false;
}

void CinderNDIReceiver::setup()
{
	if( mNdiInitialized ) {
		const NDIlib_find_create_t NDI_find_create_desc = { true, nullptr };

		mNdiFinder = NDIlib_find_create( &NDI_find_create_desc );
		if( ! mNdiFinder ) {
			CI_LOG_E( "Failed to create NDI finder!" );
		}
		
		unsigned int no_sources = 0;
		const NDIlib_source_t* p_sources = nullptr;
		while( ! no_sources ) {
			mNdiSources = NDIlib_find_get_sources( mNdiFinder, &no_sources, 10000 );
		}

		initConnection();
	}
}

void CinderNDIReceiver::initConnection()
{
	if( mNdiSources ) {
		NDIlib_recv_create_t NDI_recv_create_desc = 
		{
			mNdiSources[0],
			false,
			NDIlib_recv_bandwidth_highest,
			true 
		};

		mNdiReceiver = NDIlib_recv_create2( &NDI_recv_create_desc );
		if( ! mNdiReceiver ) {
			CI_LOG_E( "Failed to create NDI receiver!" );
		}

		const NDIlib_tally_t tally_state = { true, false };
		NDIlib_recv_set_tally( mNdiReceiver, &tally_state);

		mReadToReceive = true;
	}
}

void CinderNDIReceiver::update()
{
	// Check if we have at least one source
	unsigned int no_sources = 0;
	const NDIlib_source_t* p_sources = nullptr;
	mNdiSources = NDIlib_find_get_sources( mNdiFinder, &no_sources, 0 );

	if( ! no_sources ) {
		mReadToReceive = false;
		// Connections might take a while.. Wait for 10secs..
		while( ! no_sources ) {
			mNdiSources = NDIlib_find_get_sources( mNdiFinder, &no_sources, 10000 );
		}
	}
	else {
		// If we are here it means that the source has changed
		// so we need to rebuild our receiver.
		if( ! mReadToReceive ) {
			if( mNdiReceiver ) NDIlib_recv_destroy( mNdiReceiver );
			initConnection();
		}

	}

	if( mReadToReceive ) {

		NDIlib_video_frame_t video_frame;
		NDIlib_audio_frame_t audio_frame;
		NDIlib_metadata_frame_t metadata_frame;

		switch( NDIlib_recv_capture( mNdiReceiver, &video_frame, &audio_frame, &metadata_frame, 1000 ) )
		{	
			// No data
			case NDIlib_frame_type_none:
			{
				CI_LOG_I( "No data received. ");
				break;
			}

			// Video data
			case NDIlib_frame_type_video:
			{
				//CI_LOG_I( "Video data received with width: " << video_frame.xres << " and height: " << video_frame.yres );
				auto surface = ci::Surface::create( video_frame.p_data, video_frame.xres, video_frame.yres, video_frame.line_stride_in_bytes, ci::SurfaceChannelOrder::RGBA );
				mVideoTexture = ci::gl::Texture::create( *surface );
				mVideoTexture->setTopDown(true);
				NDIlib_recv_free_video( mNdiReceiver, &video_frame );
				break;
			}

			// Audio data
			case NDIlib_frame_type_audio:
			{
				CI_LOG_I( "Audio data received with " << audio_frame.no_samples << " number of samples" );
				NDIlib_recv_free_audio( mNdiReceiver, &audio_frame );
				break;
			}

			// Meta data
			case NDIlib_frame_type_metadata:
			{
				CI_LOG_I( "Meta data received." );
				NDIlib_recv_free_metadata( mNdiReceiver, &metadata_frame );
				break;
			}
		}
	}
}

ci::gl::Texture2dRef CinderNDIReceiver::getVideoTexture()
{
	return mVideoTexture;
}
