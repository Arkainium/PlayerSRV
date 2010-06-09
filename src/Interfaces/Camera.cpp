#include "Camera.h"
#include <algorithm>
#include <cctype>

using namespace std;
using namespace metrobotics;

Camera::Camera(PlayerSRV& playerDriver, player_devaddr_t addr)
:mPlayerDriver(playerDriver), mCameraAddr(addr)
{
	// Initial state.
	mInitialized = false;
	mActive = false;

	// Default frequency.
	mMinCycleTime = 1.0;
	mTimeElapsed = 0.0;
}

void Camera::ReadConfig(ConfigFile& cf, int section)
{
	// Read in the camera size.
	string str = cf.ReadString(section, "camera_size", "");
	transform(str.begin(), str.end(), str.begin(), (int(*)(int))tolower);
	if (str.size() == 0) {
		mCamRes = Surveyor::CAMSIZE_160x128;
		PLAYER_WARN("Camera: camera resolution has not been specified");
	} else if (str == "80x64") {
		mCamRes = Surveyor::CAMSIZE_80x64;
	} else if (str == "160x128") {
		mCamRes = Surveyor::CAMSIZE_160x128;
	} else if (str == "320x240") {
		mCamRes = Surveyor::CAMSIZE_320x240;
	} else if (str == "640x480") {
		mCamRes = Surveyor::CAMSIZE_640x480;
	} else {
		mCamRes = Surveyor::CAMSIZE_160x128;
		PLAYER_ERROR1("Camera: %s is not a valid camera resolution", str.c_str());
	}

	// Read in camera update frequency.
	mMinCycleTime = cf.ReadFloat(section, "cam_min_cycle_time", -1);
	if (mMinCycleTime < 0.0) {
		mMinCycleTime = 1.0; // seconds
	}
}

void Camera::Start()
{
	if (!mInitialized && mPlayerDriver) {
		// Apply the camera size.
		bool lockedSurveyor = false;
		try {
			Surveyor& surveyor = mPlayerDriver.LockSurveyor();
			lockedSurveyor = true;

			bool fDone = false;
			int tries = 0, maxTries = 10;
			while (!fDone && tries < maxTries) {
				try {
					surveyor.setResolution(mCamRes);
					fDone = true;
				} catch (...) {
					++tries;
				}
			}

			mPlayerDriver.UnlockSurveyor();
			lockedSurveyor = false;

			if (fDone) {
				mInitialized = true;
			} else {
				mInitialized = false;
				PLAYER_ERROR("Camera: failed to start camera");
			}
		} catch (...) {
			if (lockedSurveyor) {
				mPlayerDriver.UnlockSurveyor();
				lockedSurveyor = false;
			}
		}
	}

	if (!mActive && mInitialized && mPlayerDriver) {
		mPlayerDriver.PushCommand(TakePictureSRV(mPlayerDriver));
		mActive = true;
		mTimeElapsed = 0.0;
	}
}

void Camera::Stop()
{
	mInitialized = false;
	mActive = false;
}

void Camera::Update(double t)
{
	if (mPlayerDriver && t >= 0.0) {
		mTimeElapsed += t;
		if (!mActive && mTimeElapsed >= mMinCycleTime) {
			Start();
		}
	}
}

void Camera::Publish(const Picture& pic)
{
	if (!mPlayerDriver || !pic) {
		//* Since the driver is apparently malfunctioning, make sure not to publish
		//* anything lest we inadvertently fool the client into thinking that the 
		//* interface is fresh.
		Stop();
	} else {
		// Publish our data.
		player_camera_data_t camData;
		memset(&camData, 0, sizeof(player_camera_data_t));
		camData.format = PLAYER_CAMERA_FORMAT_RGB888;
		camData.compression = PLAYER_CAMERA_COMPRESS_JPEG;
		camData.fdiv = 1;
		camData.bpp = 24;
		camData.width = pic.width();
		camData.height = pic.height();
		camData.image_count = pic.size();
		camData.image = pic.data();
		if (camData.image) {
			mPlayerDriver.Publish(mCameraAddr, PLAYER_MSGTYPE_DATA,
			                      PLAYER_CAMERA_DATA_STATE, (void *)&camData);
		}
		mActive = false;
	}
}
