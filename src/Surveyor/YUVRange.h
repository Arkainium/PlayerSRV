#ifndef METROBOTICS_SURVEYOR_YUVRANGE_H
#define METROBOTICS_SURVEYOR_YUVRANGE_H

#include <string>
#include <sstream>
#include <ios>
#include <iomanip>
#include <cstdio>

class YUVRange
{
	public:
		YUVRange(unsigned int yMin = 0, unsigned int yMax = 0,
                 unsigned int uMin = 0, unsigned int uMax = 0,
                 unsigned int vMin = 0, unsigned int vMax = 0)
		         :mYmin(yMin), mYmax(yMax),
		          mUmin(uMin), mUmax(uMax),
		          mVmin(vMin), mVmax(vMax) { }

		YUVRange(const std::string& hex) {
			*this = YUVRange(); // Initialize to default.
			if (hex.length() >= 12) {
				sscanf(hex.substr( 0,2).c_str(), "%x", &mYmin);
				sscanf(hex.substr( 2,2).c_str(), "%x", &mYmax);
				sscanf(hex.substr( 4,2).c_str(), "%x", &mUmin);
				sscanf(hex.substr( 6,2).c_str(), "%x", &mUmax);
				sscanf(hex.substr( 8,2).c_str(), "%x", &mVmin);
				sscanf(hex.substr(10,2).c_str(), "%x", &mVmax);
			}
		}

		void setY(unsigned int min, unsigned int max) { mYmin = min; mYmax = max; }
		void setU(unsigned int min, unsigned int max) { mUmin = min; mUmax = max; }
		void setV(unsigned int min, unsigned int max) { mVmin = min; mVmax = max; }

		void getY(unsigned int &min, unsigned int &max) const { min = mYmin; max = mYmax; }
		void getU(unsigned int &min, unsigned int &max) const { min = mUmin; max = mUmax; }
		void getV(unsigned int &min, unsigned int &max) const { min = mVmin; max = mVmax; }

		unsigned int getYMin() const { return mYmin; }
		unsigned int getYMax() const { return mYmax; }
		unsigned int getUMin() const { return mUmin; }
		unsigned int getUMax() const { return mUmax; }
		unsigned int getVMin() const { return mVmin; }
		unsigned int getVMax() const { return mVmax; }

		std::string toHexString() const {
			std::stringstream ss;
			ss << std::setw(2) << std::setfill('0') << std::uppercase << std::internal << std::hex << std::noshowbase << mYmin;
			ss << std::setw(2) << std::setfill('0') << std::uppercase << std::internal << std::hex << std::noshowbase << mYmax;
			ss << std::setw(2) << std::setfill('0') << std::uppercase << std::internal << std::hex << std::noshowbase << mUmin;
			ss << std::setw(2) << std::setfill('0') << std::uppercase << std::internal << std::hex << std::noshowbase << mUmax;
			ss << std::setw(2) << std::setfill('0') << std::uppercase << std::internal << std::hex << std::noshowbase << mVmin;
			ss << std::setw(2) << std::setfill('0') << std::uppercase << std::internal << std::hex << std::noshowbase << mVmax;
			return ss.str();
		}

	private:
		unsigned int mYmin;
		unsigned int mYmax;
		unsigned int mUmin;
		unsigned int mUmax;
		unsigned int mVmin;
		unsigned int mVmax;
};

#endif
