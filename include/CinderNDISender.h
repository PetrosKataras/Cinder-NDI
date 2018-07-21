#pragma once

#include <memory>
#include <string>
#include "cinder/gl/Texture.h"
#include "Processing.NDI.Lib.h"

using NDISenderPtr = NDIlib_send_instance_t;
using NDIVideoFrame = NDIlib_video_frame_v2_t;
using NDIFrameType = NDIlib_frame_format_type_e;
const int DEFAULT_FRAMERATE_NUMERATOR = 30000;
const int DEFAULT_FRAMERATE_DENOMENATOR = 1001;

class CinderNDISender;
using CinderNDISenderPtr = std::unique_ptr<CinderNDISender>;

class CinderNDISender{
	public:
		struct Description {
			std::string 	mName; // Name of the sender.
			std::string 	mGroups; // Comma separated list of the groups the sender belongs to.
			bool 			mClockVideo{ false }; // Match video-rate to current submission frame rate.
			bool			mClockAudio{ false }; // Same for audio.
		};
		enum FrameType {
			PROGRESSIVE,
			INTERLEAVED,
			INTERLEAVED_FIELD_0,
			INTERLEAVED_FIELD_1	
		};
		struct VideoFrameParams {
			int 		mFrameRateNumerator{ DEFAULT_FRAMERATE_NUMERATOR };
			int 		mFrameRateDenomenator{ DEFAULT_FRAMERATE_DENOMENATOR };	
			int64_t		mTimecode{ INT64_MAX };
			FrameType 	mFrameType{ PROGRESSIVE };
			std::string mMetadata;
			
		};
		CinderNDISender( const Description dscr );
		~CinderNDISender();
		void send( ci::Surface* surface, const VideoFrameParams* videoFrameParams = nullptr );
	private:
		NDIlib_FourCC_type_e	getNDIColorFormatFromSurface( ci::SurfaceChannelOrder colorFormat );
		NDIFrameType			getNDIFrameType( FrameType frameType );
		NDIVideoFrame 			createVideoFrameFromSurface( ci::Surface* surface, const VideoFrameParams* videoFrameParams = nullptr );
	private:
		NDISenderPtr			mNDISender{ nullptr };
};
