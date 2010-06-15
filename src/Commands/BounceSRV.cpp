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
	try {
		Surveyor& surveyor = mPlayerDriver.LockSurveyor();

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
					try {
						Ranger& ranger = mPlayerDriver.LockRanger();
						ranger.Stop();
						mPlayerDriver.UnlockRanger();
					} catch (std::logic_error) {
						mPlayerDriver.UnlockRanger();
					}
					mPlayerDriver.UnlockSurveyor();
					return;
				}
			}
		}

		// Publish ranger data.
		try {
			Ranger& ranger = mPlayerDriver.LockRanger();
			ranger.Publish(ir);
			mPlayerDriver.UnlockRanger();
		} catch (std::logic_error) {
			mPlayerDriver.UnlockRanger();
		}
		mPlayerDriver.UnlockSurveyor();
	} catch (std::logic_error) {
		mPlayerDriver.UnlockRanger();
		mPlayerDriver.UnlockSurveyor();
	}
}
