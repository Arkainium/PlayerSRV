#include "Surveyor.h"

#include <iostream>
#include <string>
#include <sstream>
#include <vector>

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

Surveyor::Surveyor(const string& devName)
:mDevLink(devName.c_str(), B115200)
{
	// Set the global timeout to 2.6 seconds because a timed drive
	// can block all communication for up to 2.55 seconds.
	mDevLink.timeout(2600);

	// Establish a connection.
	__dbg("Surveyor: connecting to " + devName);
	sync(0); // block indefinitely until synced
	__dbg("Surveyor: connection established");
	// First sync is usually insufficient because the Surveyor
	// needs time to boot up properly.
	__dbg("Surveyor: initializing the hardware");
	sync(0); // block indefinitely until synced
	__dbg("Surveyor: initialization complete");
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
		mDevLink.flush();
		mDevLink.putByte('V');
		mDevLink.flushOutput();
		ret = mDevLink.getLine();
		if (ret.find("##Version") == string::npos) {
			throw Surveyor::OutOfSync();
		}
	} catch (PosixSerial::ReadTimeout) {
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
		mDevLink.flush();
		mDevLink.putBlock(cmd, 4);
		mDevLink.flushOutput();
		string echo;
		echo += mDevLink.getByte();
		echo += mDevLink.getByte();
		if (echo.find("#M") == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}
	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
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
		mDevLink.flush();
		mDevLink.putByte(cmd);
		mDevLink.flushOutput();
		string echo;
		echo += mDevLink.getByte();
		echo += mDevLink.getByte();
		if (echo.find(string("#") + cmd) == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}
	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
		throw Surveyor::NotResponding();
	}
}

const Picture Surveyor::takePicture()
{
	// For debugging purposes.
	stringstream signature;
	signature << "Surveyor::takePicture()";

	Picture ret;
	try {
		mDevLink.flush();
		mDevLink.putByte('I');
		mDevLink.flushOutput();
		string echo;
		echo += mDevLink.getByte(); // '#'
		echo += mDevLink.getByte(); // '#'
		echo += mDevLink.getByte(); // 'I'
		echo += mDevLink.getByte(); // 'M'
		echo += mDevLink.getByte(); // 'J'
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
		mDevLink.flush();
		mDevLink.putByte('B');
		mDevLink.flushOutput();
		string res = mDevLink.getLine();
		string key = "##BounceIR - ";
		if (res.find(key) == string::npos) {
			__dbg(signature.str() + ": incorrect acknowledgment");
			throw Surveyor::OutOfSync();
		}

		// Get the data.
		stringstream ss(res.substr(key.length()));
		int ir[4];
		ss >> ir[0];
		ss >> ir[1];
		ss >> ir[2];
		ss >> ir[3];
		if (!ss) {
			__dbg(signature.str() + ": incomplete");
			throw Surveyor::OutOfSync();
		}
		// Save the data.
		ret = IRArray(ir[0], ir[1], ir[2], ir[3]);

	} catch (PosixSerial::ReadTimeout) {
		__dbg(signature.str() + ": no response");
		throw Surveyor::NotResponding();
	}
	return ret;
}
