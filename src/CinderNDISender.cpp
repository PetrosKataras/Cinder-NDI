#include "CinderNDISender.h"
#include "cinder/Log.h"
#include "cinder/Surface.h"
#include "cinder/ip/Flip.h"
#include "Processing.NDI.Lib.h"

CinderNDISender::CinderNDISender( const Description dscr )
{
	if( ! NDIlib_initialize() ) {
		throw std::runtime_error( "Cannot run NDI on this machine. Probably unsupported CPU." );
	}
	NDIlib_send_create_t sendDscr{ dscr.mName.c_str(), dscr.mGroups.c_str(), dscr.mClockVideo, dscr.mClockAudio };
	mNDISender = NDIlib_send_create( &sendDscr );
	if( ! mNDISender ) {
		throw std::runtime_error( "Cannot create NDI Sender. NDIlib_send_create returned nullptr" );
	}	
}

CinderNDISender::~CinderNDISender()
{
	if( mNDISender ) {
		// Will also flush any pending video frames from async transimission.
		NDIlib_send_destroy( mNDISender );
		mNDISender = nullptr;
	}
	// No issue with multiple calls or still active objects.
	NDIlib_destroy();
}

void CinderNDISender::send( ci::Surface* surface, const VideoFrameParams* videoFrameParams )
{
	if( ! mNDISender || ! surface )
		return;

	auto videoFrame = createVideoFrameFromSurface( surface, videoFrameParams );	
	if( videoFrame.p_data != nullptr ) {
		NDIlib_send_send_video_async_v2( mNDISender, &videoFrame );
		//NDIlib_send_send_video_v2( mNDISender, &videoFrame );
	}
}

NDIFrameType CinderNDISender::getNDIFrameType( FrameType frameType )
{
	switch( frameType )
	{
		case PROGRESSIVE:
		{
			return NDIFrameType::NDIlib_frame_format_type_progressive;
		}
		case INTERLEAVED:
		{
			return NDIFrameType::NDIlib_frame_format_type_interleaved;
		}
		case INTERLEAVED_FIELD_0:
		{
			return NDIFrameType::NDIlib_frame_format_type_field_0;
		}
		case INTERLEAVED_FIELD_1:
		{
			return NDIFrameType::NDIlib_frame_format_type_field_1;
		}
	}
}

NDIVideoFrame CinderNDISender::createVideoFrameFromSurface( ci::Surface* surface, const VideoFrameParams* videoFrameParams )
{
	if( ! surface )
		return NDIVideoFrame();

	return { 
		surface->getWidth(),
		surface->getHeight(),
		getNDIColorFormatFromSurface( surface->getChannelOrder() ),
		videoFrameParams != nullptr ? videoFrameParams->mFrameRateNumerator : DEFAULT_FRAMERATE_NUMERATOR,
		videoFrameParams != nullptr ? videoFrameParams->mFrameRateDenomenator : DEFAULT_FRAMERATE_DENOMENATOR,
		surface->getAspectRatio(),
		videoFrameParams != nullptr ? getNDIFrameType( videoFrameParams->mFrameType ) : getNDIFrameType( FrameType::PROGRESSIVE ),
		videoFrameParams != nullptr ? videoFrameParams->mTimecode : INT64_MAX,
		surface->getData(),
		static_cast<int>( surface->getRowBytes() ),
		videoFrameParams != nullptr ? videoFrameParams->mMetadata.c_str() : nullptr,
		-1 // timestamp is only relevant on the receiver side
	};
}

NDIlib_FourCC_type_e CinderNDISender::getNDIColorFormatFromSurface( ci::SurfaceChannelOrder channelOrder ) {
	switch( channelOrder.getCode() ) {
		case ci::SurfaceChannelOrder::RGBA:
		{
			return NDIlib_FourCC_type_RGBA;
		}
		case ci::SurfaceChannelOrder::RGBX:
		{
			return NDIlib_FourCC_type_RGBX;
		}
		case ci::SurfaceChannelOrder::BGRA:
		{
			return NDIlib_FourCC_type_BGRA;
		}
		case ci::SurfaceChannelOrder::BGRX:
		{
			return NDIlib_FourCC_type_BGRX;
		}
		default:
		{
			return NDIlib_FourCC_type_RGBA;
		}
	}
}

