#include "TakePictureSRV.h"
#include "boost/thread/thread.hpp"
using namespace std;

CommandInterface* TakePictureSRV(PlayerSRV& driver)
{
	return new TakePictureSRV_Implementation(driver);
}

TakePictureSRV_Implementation::TakePictureSRV_Implementation(PlayerSRV& driver)
:mPlayerDriver(driver)
{
}

int TakePictureSRV_Implementation::priority() const
{
	// FIXME: What priority should this be?
	// Active commands should have higher priority than passive commands.
	return 10;
}

string TakePictureSRV_Implementation::id() const
{
	return string("TakePictureSRV");
}

void TakePictureSRV_Implementation::operator()()
{
	bool lockedSurveyor = false;
	bool lockedCamera = false;
	try {
		Surveyor& surveyor = mPlayerDriver.LockSurveyor();
		lockedSurveyor = true;

		Picture pic;
		while (!pic) {
			try {
				pic = surveyor.takePicture();
			} catch (...) {
				// Have we had enough yet?
				try {
					boost::this_thread::interruption_point();
				} catch (boost::thread_interrupted) {
					// Stop the camera interface.
					Camera& cam = mPlayerDriver.LockCamera();
					lockedCamera = true;
					cam.Stop();
					mPlayerDriver.UnlockCamera();
					lockedCamera = false;
					mPlayerDriver.UnlockSurveyor();
					lockedSurveyor = false;
					return;
				}
			}
		}

		// Publish camera data.
		Camera& cam = mPlayerDriver.LockCamera();
		lockedCamera = true;
		cam.Publish(pic);
		mPlayerDriver.UnlockCamera();
		lockedCamera = false;

		mPlayerDriver.UnlockSurveyor();
		lockedSurveyor = false;
	} catch (...) {
		if (lockedCamera) {
			mPlayerDriver.UnlockCamera();
			lockedCamera = false;
		}
		if (lockedSurveyor) {
			mPlayerDriver.UnlockSurveyor();
			lockedSurveyor = false;
		}
	}
}
