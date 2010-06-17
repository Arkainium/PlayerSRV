#ifndef METROBOTICS_SURVEYOR_RECT_H
#define METROBOTICS_SURVEYOR_RECT_H

#include <string>
#include <sstream>
#include <ios>
#include <iomanip>
#include <cstdio>

class Rect
{
	public:
		Rect(int x1, int x2, int y1, int y2)
		:mX1(x1), mX2(x2), mY1(y1), mY2(y2)
		{}

		Rect(const std::string& hex) {
			*this = Rect(0,0,0,0); // Initialize to zero.
			if (hex.length() >= 8) {
				sscanf(hex.substr( 0,2).c_str(), "%x", &mX1);
				sscanf(hex.substr( 2,2).c_str(), "%x", &mX2);
				sscanf(hex.substr( 4,2).c_str(), "%x", &mY1);
				sscanf(hex.substr( 6,2).c_str(), "%x", &mY2);
			}
		}

		int getX1() const { return mX1; }
		int getX2() const { return mX2; }
		int getY1() const { return mY1; }
		int getY2() const { return mY2; }

		std::string toHexString() const {
			std::stringstream ss;
			ss << std::setw(2) << std::setfill('0') << std::uppercase << std::internal << std::hex << std::noshowbase << mX1;
			ss << std::setw(2) << std::setfill('0') << std::uppercase << std::internal << std::hex << std::noshowbase << mX2;
			ss << std::setw(2) << std::setfill('0') << std::uppercase << std::internal << std::hex << std::noshowbase << mY1;
			ss << std::setw(2) << std::setfill('0') << std::uppercase << std::internal << std::hex << std::noshowbase << mY2;
			return ss.str();
		}

	private:
		int mX1;
		int mX2;
		int mY1;
		int mY2;
};

#endif
