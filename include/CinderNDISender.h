#pragma once

#include <memory>
#include <string>
#include "cinder/gl/Texture.h"
#include<Processing.NDI.Lib.h>

class CinderNDISender{
	public:
		CinderNDISender( const std::string name );
		~CinderNDISender();

		void setup();
		void sendSurface( ci::SurfaceRef surface );
	private:
		bool mNdiInitialized = false;
		NDIlib_send_instance_t mNdiSender;
		std::string mName;
};
