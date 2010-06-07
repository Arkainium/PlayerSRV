#include "BounceSRV.h"
#include "boost/thread/thread.hpp"
using namespace std;

CommandInterface* BounceSRV(PlayerSRV& driver)
{
	return new BounceSRV_Implementation(driver);
}

BounceSRV_Implementation::BounceSRV_Implementation(PlayerSRV& driver)
:mPlayerDriver(driver)
{
}

int BounceSRV_Implementation::priority() const
{
	return 10;
}

string BounceSRV_Implementation::id() const
{
	return string("BounceSRV");
}

void BounceSRV_Implementation::operator()()
{
	bool lockedSurveyor = false;
	bool lockedRanger   = false;
	try {
		Surveyor& surveyor = mPlayerDriver.LockSurveyor();
		lockedSurveyor = true;

		IRArray ir;
		bool fDone = false;
		while (!fDone) {
			try {
				ir = surveyor.bounceIR();
				fDone = true;
			} catch (...) {
				fDone = false;
				// Have we had enough yet?
				try {
					boost::this_thread::interruption_point();
				} catch (boost::thread_interrupted) {
					// Stop the ranger interface.
					Ranger& ranger = mPlayerDriver.LockRanger();
					lockedRanger = true;
					ranger.Stop();
					mPlayerDriver.UnlockRanger();
					lockedRanger = false;
					mPlayerDriver.UnlockSurveyor();
					lockedSurveyor = false;
					return;
				}
			}
		}

		// Publish ranger data.
		Ranger& ranger = mPlayerDriver.LockRanger();
		lockedRanger = true;
		ranger.Publish(ir);
		mPlayerDriver.UnlockRanger();
		lockedRanger = false;

		mPlayerDriver.UnlockSurveyor();
		lockedSurveyor = false;
	} catch (...) {
		if (lockedRanger) {
			mPlayerDriver.UnlockRanger();
			lockedRanger = false;
		}
		if (lockedSurveyor) {
			mPlayerDriver.UnlockSurveyor();
			lockedSurveyor = false;
		}
	}
}
