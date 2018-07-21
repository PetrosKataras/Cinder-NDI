#pragma once

#include <memory>
#include <string>
#include "cinder/gl/Texture.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "Processing.NDI.Lib.h"

using NDISenderPtr = NDIlib_send_instance_t;
using NDIVideoFrame = NDIlib_video_frame_v2_t;
using NDIFrameType = NDIlib_frame_format_type_e;
using NDIConnectionMeta = NDIlib_metadata_frame_t;
using NDIAudioFrame = NDIlib_audio_frame_v2_t;

const int DEFAULT_FRAMERATE_NUMERATOR = 60000;
const int DEFAULT_FRAMERATE_DENOMENATOR = 1001;

const int DEFAULT_AUDIO_SAMPLE_RATE = 48000;

class CinderNDISender;
using CinderNDISenderPtr = std::unique_ptr<CinderNDISender>;

class CinderNDISender{
	public:
		struct Description {
			std::string 	mName; // Name of the sender.
			std::string 	mGroups; // Comma separated list of the groups the sender belongs to.
			bool 			mClockVideo{ false }; // Match video-rate to current submission frame rate.
			bool			mClockAudio{ false }; // Same for audio.
			std::string		mMetadata;
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
		struct AudioFrameParams {
			int			mSampleRate{ DEFAULT_AUDIO_SAMPLE_RATE };
			int64_t		mTimecode{ INT64_MAX };
		};
		CinderNDISender( const Description dscr );
		~CinderNDISender();
		void sendSurface( ci::Surface* surface, const VideoFrameParams* videoFrameParams = nullptr );
		void sendAudio( ci::audio::Buffer* audioBuffer, const AudioFrameParams* audioFrameParams = nullptr );
	private:
		NDIlib_FourCC_type_e	getNDIColorFormatFromSurface( ci::SurfaceChannelOrder colorFormat );
		NDIFrameType			getNDIFrameType( FrameType frameType );
		NDIVideoFrame 			createVideoFrameFromSurface( ci::Surface* surface, const VideoFrameParams* videoFrameParams = nullptr );
		NDIAudioFrame			createAudioFrameFromBuffer( ci::audio::Buffer* audioBuffer, const AudioFrameParams* audioFrameParams );
	private:
		NDISenderPtr			mNDISender{ nullptr };
		Description				mSenderDescription;
};
