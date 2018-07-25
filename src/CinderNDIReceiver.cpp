#include "CinderNDIReceiver.h"
#define CI_MIN_LOG_LEVEL 2
#include "cinder/Log.h"
#include "cinder/Surface.h"
#include "cinder/gl/Sync.h"
#include "cinder/audio/Context.h"

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
	mAudioRecvThread = std::make_unique<std::thread>( std::bind( &CinderNDIReceiver::audioRecvThread, this ) );
}

CinderNDIReceiver::~CinderNDIReceiver()
{
	{
		mExitVideoThread = true;
		mVideoFramesBuffer->cancel();
		mVideoRecvThread->join();
	}
	{
		mExitAudioThread = true;
		mAudioRecvThread->join();
	}

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

void CinderNDIReceiver::audioRecvThread()
{
	while( ! mExitAudioThread ) {
		receiveAudio();
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

void CinderNDIReceiver::receiveAudio()
{
	NDIlib_audio_frame_v2_t audioFrame;
	// NDIlib_recv_capture_v2 should be safe to call at the same time from multiple threads according to the SDK.
	// e.g To capture video and audio at the same time from separate threads for example.
	// Wait max .5 sec for a new frame to arrive.
	switch( NDIlib_recv_capture_v2( mNDIReceiver, nullptr, &audioFrame, nullptr, 50 ) ) { 
		case NDIlib_frame_type_none:
		{
			CI_LOG_V( "No data available...." ); 
			break;
		}
		case NDIlib_frame_type_audio:
		{
			CI_LOG_V( "Received audio frame with no_samples : " << audioFrame.no_samples << " channels: " << audioFrame.no_channels << " channel stride: " << audioFrame.channel_stride_in_bytes ); 
			{
				std::lock_guard<std::mutex> lock( mAudioMutex );
				if( ! mCurrentAudioBuffer || mCurrentAudioBuffer->getNumChannels() != audioFrame.no_channels ) {
					auto framesPerBlock = ci::audio::Context::master()->getFramesPerBlock();
					mCurrentAudioBuffer = std::make_shared<ci::audio::Buffer>( framesPerBlock, audioFrame.no_channels );
					for( auto& buffer : mRingBuffers ) {
						buffer.clear();
					}
					mRingBuffers.clear();
					for( size_t ch = 0; ch < audioFrame.no_channels; ch++ ) {
						mRingBuffers.emplace_back( audioFrame.no_samples * audioFrame.no_channels );
					}
				}
			}
			for( size_t ch = 0; ch < audioFrame.no_channels; ch++ ) {
				mRingBuffers[ch].write( audioFrame.p_data + ch * audioFrame.no_samples, audioFrame.no_samples );
			}
			NDIlib_recv_free_audio_v2( mNDIReceiver, &audioFrame );
			break;
		}
	}
}

ci::audio::BufferRef CinderNDIReceiver::getAudioBuffer()
{
	std::lock_guard<std::mutex> lock( mAudioMutex );
	if( mCurrentAudioBuffer ) {
		for( size_t ch = 0; ch < mCurrentAudioBuffer->getNumChannels(); ch++ ) {
			if( ! mRingBuffers[ch].read( mCurrentAudioBuffer->getChannel( ch ), mCurrentAudioBuffer->getNumFrames() ) ) {
				mCurrentAudioBuffer->zero();
			}
		}
	}
	return mCurrentAudioBuffer;
}

