#include "Surveyor.h"

#include <iostream>
#include <string>
#include <sstream>

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
	// FIXME: What's a reasonable timeout?
	mDevLink.timeout(1000);
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
