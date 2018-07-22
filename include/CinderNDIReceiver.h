#pragma once

#include <memory>
#include "cinder/gl/Texture.h"
#include "CinderNDIFinder.h"

class CinderNDIReceiver;
using CinderNDIReceiverPtr = std::unique_ptr<CinderNDIReceiver>;

using NDIReceiverPtr = NDIlib_recv_instance_t;

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
	void receive();
private:
	NDIReceiverPtr mNDIReceiver;
};
