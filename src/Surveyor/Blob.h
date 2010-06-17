#ifndef METROBOTICS_SURVEYOR_BLOB_H
#define METROBOTICS_SURVEYOR_BLOB_H

#include "Rect.h"

class Blob
{
	public:
		Blob(const Rect& bbox, int hits)
		:mBBox(bbox), mHits(hits)
		{}

		Blob(const std::string& hex)
		:mBBox(hex)
		{
			if (hex.length() >= 12) {
				sscanf(hex.substr(8,4).c_str(), "%x", &mHits);
			} else {
				mHits = 0;
			}
		}

		// Test for valid blob.
		operator bool() const { return mHits != 0; }

		int getX1() const { return mBBox.getX1(); }
		int getX2() const { return mBBox.getX2(); }
		int getY1() const { return mBBox.getY1(); }
		int getY2() const { return mBBox.getY2(); }
		int getHits() const { return mHits; }

	private:
		Rect mBBox;
		int  mHits;
};

#endif
