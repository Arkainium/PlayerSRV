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
	return 25;
}

string TakePictureSRV_Implementation::id() const
{
	return string("TakePictureSRV");
}

void TakePictureSRV_Implementation::operator()()
{
	try {
		Surveyor& surveyor = mPlayerDriver.LockSurveyor();

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
					try {
						Camera& cam = mPlayerDriver.LockCamera();
						cam.Stop();
						mPlayerDriver.UnlockCamera();
					} catch (std::logic_error) {
						mPlayerDriver.UnlockCamera();
					}
					// Stop the metrocam interface.
					try {
						MetroCam& cam = mPlayerDriver.LockMetroCam();
						cam.Stop();
						mPlayerDriver.UnlockMetroCam();
					} catch (std::logic_error) {
						mPlayerDriver.UnlockMetroCam();
					}
					mPlayerDriver.UnlockSurveyor();
					return;
				}
			}
		}

		// Publish camera data.
		try {
			Camera& cam = mPlayerDriver.LockCamera();
			cam.Publish(pic);
			mPlayerDriver.UnlockCamera();
		} catch (std::logic_error) {
			mPlayerDriver.UnlockCamera();
		}
		// Publish metrocam data.
		try {
			MetroCam& cam = mPlayerDriver.LockMetroCam();
			cam.Publish(pic);
			mPlayerDriver.UnlockMetroCam();
		} catch (std::logic_error) {
			mPlayerDriver.UnlockMetroCam();
		}
		mPlayerDriver.UnlockSurveyor();
	} catch (std::logic_error) {
		mPlayerDriver.UnlockCamera();
		mPlayerDriver.UnlockMetroCam();
		mPlayerDriver.UnlockSurveyor();
	}
}
