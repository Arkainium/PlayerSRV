#include "Surveyor.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>

using namespace std;
using namespace metrobotics;

// Debugging is off by default.
static bool fDebugSurveyor = false;
static void __dbg(const string& msg)
{
	if (fDebugSurveyor) {
		cerr << msg << endl;
	}
}
void Surveyor::debugging(bool flag)
{
	fDebugSurveyor = flag;
}

// Initialize physical dimensions that are common to all Surveyors.
const double Surveyor::gLength = 0.1270; // 5 inches.
const double Surveyor::gWidth  = 0.1016; // 4 inches.
const double Surveyor::gHeight = 0.0800; // π inches.

Surveyor::Surveyor(const string& devName, CameraResolution res)
:mDevLink(devName.c_str(), B115200)
{
	// Default timeouts (in milliseconds).
	mMaxTimeout = 3000;
	mMinTimeout = 600;
	mDevLink.timeout(mMaxTimeout);

	// Establish a connection.
	__dbg("Surveyor: connecting to " + devName);
	sync(0); // block indefinitely until synced
	__dbg("Surveyor: connection established");
	// First sync is usually insufficient because the Surveyor
	// needs time to boot up properly.
	__dbg("Surveyor: initializing the hardware");
	sync(0); // block indefinitely until synced
	__dbg("Surveyor: initialization complete");

	// Set the camera resolution for the first time.
	bool fDone = false;
	while (!fDone) {
		try {
			setResolution(res);
			fDone = true;
		} catch (InvalidResolution) {
			__dbg("Surveyor: invalid camera resolution");
			throw;
		} catch (OutOfSync) {
			fDone = false;
		} catch (NotResponding) {
			fDone = false;
		}
	}
	__dbg("Surveyor: hardware is fully operational");
}

bool Surveyor::sync(unsigned int maxTries)
{
	unsigned int tries = 0;
	while (maxTries == 0 || tries < maxTries) {
		try {
			if (getVersion().size() > 0) {
				return true;
			}
		} catch (Surveyor::OutOfSync) {
			// Surveyor seems to be alive, but it's returning gibberish.
			__dbg("Surveyor: out of sync");
		} catch (Surveyor::NotResponding) {
			__dbg("Surveyor: not responding");
		}
		++tries;
	}
	return false;
}

string Surveyor::getVersion()
{
	string ret;
	try {
		// This is a quick command.
		// Reduce the timeout temporarily to increase performance.
		mDevLink.timeout(mMinTimeout);
		mDevLink.flush();
		mDevLink.putByte('V');
		mDevLink.flushOutput();
		ret = mDevLink.getLine();
		mDevLink.timeout(mMaxTimeout);
		if (ret.find("##Version") == string::npos) {
			throw Surveyor::OutOfSync();
		}
	} catch (PosixSerial::ReadTimeout) {
		mDevLink.timeout(mMaxTimeout);
		throw Surveyor::NotResponding();
	}
	return ret;
}

void Surveyor::drive(int l, int r, int t)
{
	/* A 4 character command sequence:
	 *     First  Character: 'M'
	 *     Second Character: left  tread speed
	 *     Third  Character: right tread speed
	 *     Fourth Character: duration in tens of milliseconds
	 *                       duration of zero (the default) means unlimited
	 *                       that is until the next drive command
	 */

	// For debugging purposes.
	stringstream signature;
	signature << "Surveyor::drive(" << l << ", " << r << ", " << t << ")";

	// Ensure the arguments are valid.
	if (l < -127 || l > 127 || r < -127 || r > 127) {
		__dbg(signature.str() + ": invalid speed");
		throw InvalidSpeed();
	}
	if (t < 0 || t > 255) {
		__dbg(signature.str() + ": invalid duration");
		throw InvalidDuration();
	}

	// Prepare the command.
	unsigned char cmd[4];
	cmd[0] = 'M';
	cmd[1] = l & 0xFF;
	cmd[2] = r & 0xFF;
	cmd[3] = t & 0xFF;

	try {
		if (t == 0) {
			// This is not a timed drive.
			// Reduce timeout to increase performance.
			mDevLink.timeout(mMinTimeout);
		}
		mDevLink.flush();
		mDevLink.putBlock(cmd, 4);
		mDevLink.flushOutput();
		string echo;
		echo += mDevLink.getByte();
		echo += mDevLink.getByte();
		mDevLink.timeout(mMaxTimeout);
		if (echo.find("#M") == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}
	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
		mDevLink.timeout(mMaxTimeout);
		throw Surveyor::NotResponding();
	}
}

void Surveyor::setResolution(CameraResolution res)
{
	// For debugging purposes.
	stringstream signature;
	signature << "Surveyor::setResolution()";

	// Determine which command to send.
	char cmd;
	switch (res) {
		case CAMSIZE_80x64:
			cmd = 'a';
			break;
		case CAMSIZE_160x128:
			cmd = 'b';
			break;
		case CAMSIZE_320x240:
			cmd = 'c';
			break;
		case CAMSIZE_640x480:
			cmd = 'A';
			break;
		default:
			__dbg(signature.str() + ": invalid camera resolution");
			throw Surveyor::InvalidResolution();
			break;
	}

	// Make it so.
	try {
		// Reduce timeout to increase performance.
		mDevLink.timeout(mMinTimeout);
		mDevLink.flush();
		mDevLink.putByte(cmd);
		mDevLink.flushOutput();
		string echo;
		echo += mDevLink.getByte();
		echo += mDevLink.getByte();
		mDevLink.timeout(mMaxTimeout);
		if (echo.find(string("#") + cmd) == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}
	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
		mDevLink.timeout(mMaxTimeout);
		throw Surveyor::NotResponding();
	}
	mCurrentRes = res;
}

const Picture Surveyor::takePicture()
{
	// For debugging purposes.
	stringstream signature;
	signature << "Surveyor::takePicture()";


	Picture ret;
	try {
		// Taking pictures is buggy with the Surveyor.
		// Reduce the timeout, and try more often.
		mDevLink.timeout(mMinTimeout);
		mDevLink.flush();
		mDevLink.putByte('I');
		mDevLink.flushOutput();
		string echo;
		echo += mDevLink.getByte(); // '#'
		echo += mDevLink.getByte(); // '#'
		echo += mDevLink.getByte(); // 'I'
		echo += mDevLink.getByte(); // 'M'
		echo += mDevLink.getByte(); // 'J'
		mDevLink.timeout(mMaxTimeout);
		if (echo.find("##IMJ") == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}

		// Get the frame size in pixels.
		unsigned char frame_resolution = mDevLink.getByte();
		size_t width = 0, height = 0; // Dimensions.
		switch (frame_resolution) {
			case '1': {
				width  = 80;
				height = 64;
			} break;
			case '3': {
				width  = 160;
				height = 128;
			} break;
			case '5': {
				width  = 320;
				height = 240;
			} break;
			case '7': {
				width  = 640;
				height = 480;
			} break;
		}

		// Get the frame size in bytes.
		unsigned long frame_size = mDevLink.getByte();
		frame_size += (mDevLink.getByte() * 256);
		frame_size += (mDevLink.getByte() * 256 * 256);
		frame_size += (mDevLink.getByte() * 256 * 256 * 256);

		// Image data.
		unsigned char *frame_buffer = new unsigned char[frame_size];
		if (!frame_buffer) {
			__dbg(signature.str() + ": failed to allocate memory for the image data");
			throw Surveyor::OutOfMemory();
		} else {
			try {
				// Read it.
				mDevLink.getBlock(frame_buffer, frame_size);
			} catch (...) {
				delete[] frame_buffer;
				throw;
			}
			// Save it.
			ret = Picture(frame_buffer, frame_size, width, height);
		}
	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
		mDevLink.timeout(mMaxTimeout);
		throw Surveyor::NotResponding();
	}
	return ret;
}

const IRArray Surveyor::bounceIR()
{
	// For debugging purposes.
	stringstream signature;
	signature << "Surveyor::bounceIR()";

	IRArray ret;
	try {
		// Reduce timeout to increase performance.
		mDevLink.timeout(mMinTimeout);
		mDevLink.flush();
		mDevLink.putByte('B');
		mDevLink.flushOutput();
		string res = mDevLink.getLine();
		string key = "##BounceIR - ";
		mDevLink.timeout(mMaxTimeout);
		if (res.find(key) == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}

		// Get the data.
		string irdata = res.substr(key.length());
		unsigned int ir[4];
		sscanf(irdata.c_str(), "%x %x %x %x", &ir[0], &ir[1], &ir[2], &ir[3]);
		// Save the data.
		ret = IRArray(ir[0], ir[1], ir[2], ir[3]);

	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
		mDevLink.timeout(mMaxTimeout);
		throw Surveyor::NotResponding();
	}
	return ret;
}

const YUVRange Surveyor::getColorBin(int bin)
{
	// For debugging purposes.
	stringstream signature;
	signature << "Surveyor::getColorBin()";

	if (bin < 0 || bin > 15) {
		__dbg(signature.str() + ": invalid color bin");
		throw InvalidColorBin();
	}

	// Prepare the command.
	unsigned char cmd[3];
	cmd[0] = 'v';
	cmd[1] = 'r';
	cmd[2] = '0' + bin;

	YUVRange ret;
	try {
		// Reduce timeout to increase performance.
		mDevLink.timeout(mMinTimeout);
		mDevLink.flush();
		mDevLink.putBlock(cmd, 3);
		mDevLink.flushOutput();
		string res = mDevLink.getLine();
		string key = "##vr"; key += cmd[2];
		mDevLink.timeout(mMaxTimeout);
		if (res.find(key) == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}
		// Extract the YUV data.
		ret = YUVRange(res.substr(key.length()));
	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
		mDevLink.timeout(mMaxTimeout);
		throw Surveyor::NotResponding();
	}
	return ret;
}

void Surveyor::setColorBin(int bin, const YUVRange& color)
{
	// For debugging purposes.
	stringstream signature;
	signature << "Surveyor::setColorBin()";

	if (bin < 0 || bin > 15) {
		__dbg(signature.str() + ": invalid color bin");
		throw InvalidColorBin();
	}

	// Prepare the command.
	unsigned char cmd[15];
	cmd[0] = 'v';
	cmd[1] = 'c';
	cmd[2] = '0' + bin;
	memcpy(cmd + 3, color.toHexString().c_str(), 12);

	try {
		// Reduce timeout to increase performance.
		mDevLink.timeout(mMinTimeout);
		mDevLink.flush();
		mDevLink.putBlock(cmd, 15);
		mDevLink.flushOutput();
		string res = mDevLink.getLine();
		string key = "##vc"; key += cmd[2];
		mDevLink.timeout(mMaxTimeout);
		if (res.find(key) == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}
	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
		mDevLink.timeout(mMaxTimeout);
		throw Surveyor::NotResponding();
	}
}

const YUVRange Surveyor::grabColorBin(int bin, const Rect& rect)
{
	// For debugging purposes.
	stringstream signature;
	signature << "Surveyor::grabColorBin()";

	if (bin < 0 || bin > 15) {
		__dbg(signature.str() + ": invalid color bin");
		throw InvalidColorBin();
	}

	// Check bounds of the rectangular region.
	if (rect.getX1() < 0 || rect.getX1() > 255 ||
	    rect.getX2() < 0 || rect.getX2() > 255 ||
	    rect.getY1() < 0 || rect.getY1() > 255 ||
	    rect.getY2() < 0 || rect.getY2() > 255) {
		__dbg(signature.str() + ": invalid rectangular region");
		throw InvalidRect();
	}

	// Prepare the command.
	unsigned char cmd[11];
	cmd[0] = 'v';
	cmd[1] = 'g';
	cmd[2] = '0' + bin;
	memcpy(cmd + 3, rect.toHexString().c_str(), 8);

	YUVRange ret;
	try {
		// Reduce timeout to increase performance.
		mDevLink.timeout(mMinTimeout);
		mDevLink.flush();
		mDevLink.putBlock(cmd, 11);
		mDevLink.flushOutput();
		string res = mDevLink.getLine();
		string key = "##vg"; key += cmd[2];
		mDevLink.timeout(mMaxTimeout);
		if (res.find(key) == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}
		// Extract the YUV data.
		ret = YUVRange(res.substr(key.length()));
	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
		mDevLink.timeout(mMaxTimeout);
		throw Surveyor::NotResponding();
	}
	return ret;
}

void Surveyor::getBlobs(int bin, list<Blob>& blobs)
{
	// For debugging purposes.
	stringstream signature;
	signature << "Surveyor::getBlobs()";

	if (bin < 0 || bin > 15) {
		__dbg(signature.str() + ": invalid color bin");
		throw InvalidColorBin();
	}

	// Prepare the command.
	unsigned char cmd[3];
	cmd[0] = 'v';
	cmd[1] = 'b';
	cmd[2] = '0' + bin;

	try {
		// Reduce timeout to increase performance.
		mDevLink.timeout(mMinTimeout);
		mDevLink.flush();
		mDevLink.putBlock(cmd, 3);
		mDevLink.flushOutput();
		string res = mDevLink.getLine();
		string key = "##vb"; key += cmd[2];
		mDevLink.timeout(mMaxTimeout);
		if (res.find(key) == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}
		// Extract the blob data.
		blobs.clear();
		for (string::size_type i = 5; (i + 12) < res.length(); i += 12) {
			string blobStr = res.substr(i, 12);
			Blob blob(blobStr);
			if (blob) {
				blobs.push_back(blob);
			}
		}
	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
		mDevLink.timeout(mMaxTimeout);
		throw Surveyor::NotResponding();
	}
}

void Surveyor::scanColumns(int bin, vector<int>& cols, ScanType type)
{
	// For debugging purposes.
	stringstream signature;
	signature << "Surveyor::scanColumns()";

	if (bin < 0 || bin > 15) {
		__dbg(signature.str() + ": invalid color bin");
		throw InvalidColorBin();
	}

	// Prepare the command.
	unsigned char cmd[3];
	cmd[0] = 'v';
	switch (type) {
		case SCAN_FIRST_MATCH: {
			cmd[1] = 'f';
		} break;
		case SCAN_FIRST_MISMATCH: {
			cmd[1] = 's';
		} break;
		case SCAN_MATCH_COUNT: {
			cmd[1] = 'n';
		} break;
		default: {
			__dbg(signature.str() + ": invalid scan type");
			throw InvalidScanType();
		} break;
	}
	cmd[2] = '0' + bin;

	try {
		// Reduce timeout to increase performance.
		mDevLink.timeout(mMinTimeout);
		mDevLink.flush();
		mDevLink.putBlock(cmd, 3);
		mDevLink.flushOutput();
		string res = mDevLink.getLine();
		string key = "##v"; key += cmd[1]; key += cmd[2];
		mDevLink.timeout(mMaxTimeout);
		if (res.find(key) == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}
		// Extract the column data.
		cols.clear(); cols.reserve(80);
		for (string::size_type i = 5; (i + 2) < res.length(); i += 2) {
			int value = 0;
			sscanf(res.substr(i, 2).c_str(), "%x", &value);
			cols.push_back(value);
		}
	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
		mDevLink.timeout(mMaxTimeout);
		throw Surveyor::NotResponding();
	}
}
