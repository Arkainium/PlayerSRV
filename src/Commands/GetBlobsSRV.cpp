#include "GetBlobsSRV.h"
#include "boost/thread/thread.hpp"
#include <list>
using namespace std;

CommandInterface* GetBlobsSRV(PlayerSRV& driver, int channel)
{
	return new GetBlobsSRV_Implementation(driver, channel);
}

GetBlobsSRV_Implementation::GetBlobsSRV_Implementation(PlayerSRV& driver, int channel)
:mPlayerDriver(driver), mChannel(channel)
{
	// Keep the channel within the bounds of Surveyor capability.
	if (mChannel < 0 || mChannel > 15) {
		mChannel = 0;
	}
}

int GetBlobsSRV_Implementation::priority() const
{
	// FIXME: What priority should this be?
	// Active commands should have higher priority than passive commands.
	return 50;
}

string GetBlobsSRV_Implementation::id() const
{
	return string("GetBlobsSRV");
}

void GetBlobsSRV_Implementation::operator()()
{
	try {
		Surveyor& surveyor = mPlayerDriver.LockSurveyor();

		list<Blob> blobs;
		bool fDone = false;
		while (!fDone) {
			try {
				surveyor.getBlobs(mChannel, blobs);
				fDone = true;
			} catch (...) {
				fDone = false;
				// Have we had enough yet?
				try {
					boost::this_thread::interruption_point();
				} catch (boost::thread_interrupted) {
					// Stop the blobfinder interface.
					try {
						BlobFinder& bf = mPlayerDriver.LockBlobFinder();
						bf.Stop();
						mPlayerDriver.UnlockBlobFinder();
					} catch (std::logic_error) {
						mPlayerDriver.UnlockBlobFinder();
					}
					mPlayerDriver.UnlockSurveyor();
					return;
				}
			}
		}

		// Publish blob data.
		try {
			BlobFinder& bf = mPlayerDriver.LockBlobFinder();
			bf.Publish(mChannel, blobs);
			mPlayerDriver.UnlockBlobFinder();
		} catch (std::logic_error) {
			mPlayerDriver.UnlockBlobFinder();
		}
		mPlayerDriver.UnlockSurveyor();
	} catch (std::logic_error) {
		mPlayerDriver.UnlockBlobFinder();
		mPlayerDriver.UnlockSurveyor();
	}
}
