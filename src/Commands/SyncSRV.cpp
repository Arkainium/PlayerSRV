#include "SyncSRV.h"
#include "boost/thread/thread.hpp"
using namespace std;

CommandInterface* SyncSRV(PlayerSRV& driver)
{
	return new SyncSRV_Implementation(driver);
}

SyncSRV_Implementation::SyncSRV_Implementation(PlayerSRV& driver)
:mPlayerDriver(driver)
{
}

int SyncSRV_Implementation::priority() const
{
	// This really should have the highest possible priority.
	return 999999999;
}

string SyncSRV_Implementation::id() const
{
	return string("SyncSRV");
}

void SyncSRV_Implementation::operator()()
{
	Surveyor& surveyor = mPlayerDriver.LockSurveyor();

	bool fDone = false;
	while (!fDone) {
		try {
			fDone = surveyor.sync(1);
		} catch (...) {
			fDone = false;
			// Have we had enough yet?
			try {
				boost::this_thread::interruption_point();
			} catch (boost::thread_interrupted) {
				fDone = true;
			}
		}
	}

	mPlayerDriver.UnlockSurveyor();
}
