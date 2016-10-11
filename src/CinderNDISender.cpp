#include "CinderNDISender.h"

#include "cinder/Log.h"
#include "cinder/Surface.h"
#include <Processing.NDI.Recv.h>

CinderNDISender::CinderNDISender( const std::string name )
: mName( name )	
{
	if( ! NDIlib_is_supported_CPU() ) {
		CI_LOG_E( "Failed to initialize NDI because of unsupported CPU!" );
	}

	if( ! NDIlib_initialize() ) {
		CI_LOG_E( "Failed to initialize NDI!" );
	}

	mNdiInitialized = true;
}

CinderNDISender::~CinderNDISender()
{
	if( mNdiSender ) {
		NDIlib_send_destroy( mNdiSender );
	}
	NDIlib_destroy();
	mNdiInitialized = false;
}

void CinderNDISender::setup()
{
	if( mNdiInitialized ) {
		
		NDIlib_send_create_t NDI_send_create_desc = { mName.c_str(), nullptr, true, false };
		mNdiSender = NDIlib_send_create( &NDI_send_create_desc );
	}
}

void CinderNDISender::sendSurface( ci::SurfaceRef surface )
{
	if( surface ) {

		if( NDIlib_send_get_no_connections( mNdiSender, 10000 ) ) {

			NDIlib_tally_t NDI_tally;
			NDIlib_send_get_tally( mNdiSender, &NDI_tally, 0 );  

			const NDIlib_video_frame_t NDI_video_frame = {
				(unsigned int)( surface->getWidth() ),
				(unsigned int)( surface->getHeight() ),
				NDIlib_FourCC_type_BGRA,
				60000, 1001,
				(float)surface->getWidth()/(float)surface->getHeight(),
				true,
				NDIlib_send_timecode_synthesize,
				surface->getData(),
				(unsigned int)( surface->getRowBytes() )
			};

			NDIlib_send_send_video( mNdiSender, &NDI_video_frame );
		}
	}
}

