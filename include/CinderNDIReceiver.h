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
		bool mNdiInitialized = false;
		bool mReadToReceive = false;
		NDIlib_recv_instance_t mNdiReceiver;
		ci::gl::Texture2dRef mVideoTexture;

};
