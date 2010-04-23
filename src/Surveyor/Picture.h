#ifndef METROBOTICS_SURVEYOR_PICTURE_H
#define METROBOTICS_SURVEYOR_PICTURE_H

#include "boost/shared_ptr.hpp"

class Picture
{
	public:
		Picture() { } // Null picture.
		Picture(unsigned char* buf, size_t size, size_t width, size_t height)
			:mData(buf), mSize(size), mWidth(width), mHeight(height) { }

		// Test for null picture.
		operator bool() const { return mData; }

		// Picture dimensions.
		const size_t width() const { return mWidth; }
		const size_t height() const { return mHeight; }

		// Picture size in bytes.
		const size_t size() const { return mSize; }

		// Image data.
		unsigned char* data() const { return mData.get(); }

	private:
		boost::shared_ptr<unsigned char> mData;
		size_t mSize, mWidth, mHeight;
};

#endif
