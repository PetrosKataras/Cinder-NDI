#pragma once

#include <memory>
#include<Processing.NDI.Lib.h>
#include "cinder/gl/Texture.h"

class CinderNDIReceiver{
	public:
		CinderNDIReceiver();
		~CinderNDIReceiver();

		void setup();
		void update();
		ci::gl::Texture2dRef getVideoTexture();
	private:
		void initConnection();
	private:
		bool mNdiInitialized = false;
		bool mReadToReceive = false;
		ci::gl::Texture2dRef mVideoTexture;

		NDIlib_recv_instance_t mNdiReceiver;
		NDIlib_find_instance_t mNdiFinder;
		const NDIlib_source_t* mNdiSources = nullptr; // Owned by NDI.

};
