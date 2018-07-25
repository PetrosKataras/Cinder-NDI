#pragma once

#include <memory>
#include "cinder/gl/Texture.h"
#include "cinder/gl/Context.h"
#include "cinder/ConcurrentCircularBuffer.h"
#include "cinder/audio/Buffer.h"
#include "cinder/audio/dsp/RingBuffer.h"
#include "CinderNDIFinder.h"

class CinderNDIReceiver;
using CinderNDIReceiverPtr = std::unique_ptr<CinderNDIReceiver>;

using NDIReceiverPtr = NDIlib_recv_instance_t;

using VideoFramesBuffer = ci::ConcurrentCircularBuffer<ci::gl::TextureRef>;
using VideoFramesBufferPtr = std::unique_ptr<VideoFramesBuffer>;

using AudioFramesBuffer = ci::ConcurrentCircularBuffer<ci::audio::BufferRef>;
using AudioFramesBufferPtr = std::unique_ptr<AudioFramesBuffer>;

class CinderNDIReceiver{
public:
	enum Bandwidth {
		METADATA_ONLY = NDIlib_recv_bandwidth_metadata_only,
		AUDIO_ONLY = NDIlib_recv_bandwidth_audio_only,
		LOWEST = NDIlib_recv_bandwidth_lowest,
		HIGHEST = NDIlib_recv_bandwidth_highest,
	};
	enum ColorFormat {
		BGRX_BGRA = NDIlib_recv_color_format_BGRX_BGRA,
		UYVY_BGRA = NDIlib_recv_color_format_UYVY_BGRA,
		RGBX_RGBA = NDIlib_recv_color_format_RGBX_RGBA,
		UYVY_RGBA = NDIlib_recv_color_format_UYVY_RGBA,
		FASTEST = NDIlib_recv_color_format_fastest
	};
	struct Description {
		ColorFormat mColorFormat{ RGBX_RGBA };
		Bandwidth mBandwidth{ HIGHEST };
		bool mAllowVideoFields{ false };
		const NDISource* source{ nullptr }; // Owened by NDIlib_find
		std::string mName;
	};
	CinderNDIReceiver( const Description dscr );
	~CinderNDIReceiver();
	void connect( const NDISource& source );
	void disconnect();
	ci::gl::TextureRef getVideoTexture();
	ci::audio::BufferRef getAudioBuffer();
private:
	void videoRecvThread( ci::gl::ContextRef ctx );
	void receiveVideo();
	void audioRecvThread();
	void receiveAudio();
private:
	NDIReceiverPtr					mNDIReceiver;

	VideoFramesBufferPtr			mVideoFramesBuffer;
	std::unique_ptr<std::thread> 	mVideoRecvThread;
	ci::gl::TextureRef				mVideoTexture;
	
	std::unique_ptr<std::thread> 	mAudioRecvThread;
	ci::audio::BufferRef			mCurrentAudioBuffer;
	std::vector<ci::audio::dsp::RingBuffer> 		mRingBuffers;
	std::mutex						mAudioMutex;
	bool							mExitVideoThread{ false };
	bool							mExitAudioThread{ false };
};
