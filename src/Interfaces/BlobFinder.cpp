#include "BlobFinder.h"

using namespace std;
using namespace metrobotics;

BlobFinder::BlobFinder(PlayerSRV& playerDriver, player_devaddr_t addr)
:mPlayerDriver(playerDriver), mBlobFinderAddr(addr)
{
	// Clear our data structures.
	memset(&mBlobFinderData, 0, sizeof(player_blobfinder_data_t));
	memset(&mBlobFinderBlobs, 0, MAX_BLOBS*sizeof(player_blobfinder_blob_t));
	memset(&mBlobFinderConfig, 0, MAX_BLOBS*sizeof(player_blobfinder_color_config_t));
	memset(&mBlobFinderColors, 0, MAX_BLOBS*sizeof(uint32_t));

	// Initial state.
	mActiveChannel = 0;
	mActive = false;
	for (int i = 0; i < MAX_BLOBS; ++i) {
		mInitialized[i] = false;
	}

	// Default frequency.
	mMinCycleTime = 1.0;
	mTimeElapsed = 0.0;
}

void BlobFinder::ReadConfig(ConfigFile& cf, int section)
{
	// Read in blobfinder update frequency.
	mMinCycleTime = cf.ReadFloat(section, "blobfinder_min_cycle_time", -1);
	if (mMinCycleTime < 0.0) {
		mMinCycleTime = 1.0; // seconds
	}

	// Read in blobfinder channel configuration.
	int numEntries = cf.GetTupleCount(section, "blobfinder_config_channels");
	int numChannels = numEntries / 3;
	if (numChannels < 1) {
		PLAYER_ERROR("BlobFinder: missing channel configuration");
	} else {
		for (int i = 0; i < numChannels; ++i) {
			int j = i * 3;
			int channel   = cf.ReadTupleInt(section,    "blobfinder_config_channels", j+0, -1);
			string rgbStr = cf.ReadTupleString(section, "blobfinder_config_channels", j+1, "");
			string yuvStr = cf.ReadTupleString(section, "blobfinder_config_channels", j+2, "");
			if (channel >= 0 && channel <= 15 && rgbStr.size() > 0 && yuvStr.size() > 0) {
				YUVRange yuv(yuvStr);
				mBlobFinderConfig[channel].channel = channel;
				mBlobFinderConfig[channel].rmin = yuv.getYMin();
				mBlobFinderConfig[channel].rmax = yuv.getYMax();
				mBlobFinderConfig[channel].gmin = yuv.getUMin();
				mBlobFinderConfig[channel].gmax = yuv.getUMax();
				mBlobFinderConfig[channel].bmin = yuv.getVMin();
				mBlobFinderConfig[channel].bmax = yuv.getVMax();
				sscanf(rgbStr.c_str(), "%x", &mBlobFinderColors[channel]);
			} else {
				PLAYER_WARN("BlobFinder: incorrect channel configuration");
			}
		}
	}
}

int BlobFinder::ProcessMessage(QueuePointer& queue, player_msghdr *msghdr, void *data)
{
	if (msghdr->type == PLAYER_MSGTYPE_REQ) {
		// Request subtypes.
		switch (msghdr->subtype) {
			case PLAYER_BLOBFINDER_REQ_SET_COLOR: {
				// Use this to change the color channel; ignore the rest.
				int newChannel = ((player_blobfinder_color_config_t*)data)->channel;
				if (newChannel >= 0 && newChannel <= 15) {
					mActiveChannel = newChannel;
				}
				// Acknowledge the request.
				mPlayerDriver.Publish(mBlobFinderAddr, queue, PLAYER_MSGTYPE_RESP_ACK, msghdr->subtype);
				return 0;
			} break;
			case PLAYER_BLOBFINDER_REQ_SET_IMAGER_PARAMS: {
				// Surveyor doesn't do this; just acknowledge the request and move on with life.
				mPlayerDriver.Publish(mBlobFinderAddr, queue, PLAYER_MSGTYPE_RESP_ACK, msghdr->subtype);
				return 0;
			} break;
			default: break;
		}
	}
	return -1;
}

void BlobFinder::Start()
{
	if (!mInitialized[mActiveChannel] && mPlayerDriver) {
		// Set up the current channel.
		const player_blobfinder_color_config_t& conf = mBlobFinderConfig[mActiveChannel];
		const YUVRange yuvRange(conf.rmin, conf.rmax, conf.gmin, conf.gmax, conf.bmin, conf.bmax);
		try {
			Surveyor& surveyor = mPlayerDriver.LockSurveyor();
			bool fDone = false;
			int tries = 0, maxTries = 10;
			while (!fDone && tries < maxTries) {
				try {
					surveyor.setColorBin(mActiveChannel, yuvRange);
					fDone = true;
				} catch (...) {
					fDone = false;
					++tries;
				}
			}
			mPlayerDriver.UnlockSurveyor();
			if (fDone) {
				mInitialized[mActiveChannel] = true;
			} else {
				mInitialized[mActiveChannel] = false;
				PLAYER_ERROR("BlobFinder: failed to initialize color bin");
			}
		} catch (...) {
			mPlayerDriver.UnlockSurveyor();
		}
	}

	if (!mActive && mInitialized[mActiveChannel] && mPlayerDriver) {
		mPlayerDriver.PushCommand(GetBlobsSRV(mPlayerDriver, mActiveChannel));
		mActive = true;
		mTimeElapsed = 0.0;
	}
}

void BlobFinder::Stop()
{
	mActive = false;
	// We can't trust the initialized state any more; reset them all.
	for (int i = 0; i < MAX_BLOBS; ++i) {
		mInitialized[i] = false;
	}
}

void BlobFinder::Update(double t)
{
	if (mPlayerDriver && t >= 0.0) {
		mTimeElapsed += t;
		if (!mActive && mTimeElapsed >= mMinCycleTime) {
			Start();
		}
	}
}

void BlobFinder::Publish(int channel, const list<Blob>& blobs)
{
	if (!mPlayerDriver) {
		//* Since the driver is apparently malfunctioning, make sure not to publish
		//* anything lest we inadvertently fool the client into thinking that the 
		//* interface is fresh.
		Stop();
	} else {
		// Retrieve the data.
		// Surveyor's blob image window is always the same.
		mBlobFinderData.width  = BLOB_WINDOW_WIDTH;
		mBlobFinderData.height = BLOB_WINDOW_HEIGHT;
		// Clear out the old blob data.
		memset(&mBlobFinderBlobs, 0, MAX_BLOBS*sizeof(player_blobfinder_blob_t));
		// Fill in the new blob data.
		mBlobFinderData.blobs_count = blobs.size();
		int blob_id = 0;
		for (list<Blob>::const_iterator iter = blobs.begin(); iter != blobs.end(); ++iter) {
			mBlobFinderBlobs[blob_id].id     = blob_id;
			mBlobFinderBlobs[blob_id].color  = mBlobFinderColors[channel];
			mBlobFinderBlobs[blob_id].area   = iter->getHits();
			mBlobFinderBlobs[blob_id].left   = iter->getX1();
			mBlobFinderBlobs[blob_id].right  = iter->getX2();
			mBlobFinderBlobs[blob_id].top    = BLOB_WINDOW_HEIGHT - iter->getY2() - 1;
			mBlobFinderBlobs[blob_id].bottom = BLOB_WINDOW_HEIGHT - iter->getY1() - 1;
			++blob_id;
		}
		mBlobFinderData.blobs = (player_blobfinder_blob_t*)&mBlobFinderBlobs;
		// Publish our data.
		mPlayerDriver.Publish(mBlobFinderAddr, PLAYER_MSGTYPE_DATA,
		                      PLAYER_BLOBFINDER_DATA_BLOBS, (void *)&mBlobFinderData);
		mActive = false;
	}
}
