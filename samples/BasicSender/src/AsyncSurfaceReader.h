#pragma once

#include "cinder/gl/Fbo.h"
#include "cinder/gl/Pbo.h"
#include "cinder/Surface.h"

class AsyncSurfaceReader {
public:
	AsyncSurfaceReader( const int width, const int height );
	ci::SurfaceRef readPixels();
	void bind();
	void unbind();
	const int getWidth() { return mWidth; }
	const int getHeight() { return mHeight; }
private:
	ci::gl::FboRef mFbo[2];
	ci::gl::PboRef mPbo[2];
	ci::SurfaceRef mSurface[2];
	uint8_t mFrontIndex{ 1 };
	uint8_t mBackIndex{ 1 };
	int mWidth{ -1 };
	int mHeight{ -1 };
};
