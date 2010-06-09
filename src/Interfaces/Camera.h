#ifndef METROBOTICS_PLAYERSRV_INTERFACE_CAMERA_H
#define METROBOTICS_PLAYERSRV_INTERFACE_CAMERA_H

/* Forward declaration of our driver. */
#include "../PlayerSRV.h"
class PlayerSRV;

class Camera
{
	public:
		//* Construct a new camera interface,
		//* and bind it to the PlayerSRV driver.
		Camera(PlayerSRV& driver, player_devaddr_t addr);

		//* Extract any relevant information from the config file.
		void ReadConfig(ConfigFile& cf, int section);

		//* Address to which the interface is bound on the server.
		player_devaddr_t Address() const { return mCameraAddr; }

		//* State management.
		bool Active() const { return mActive; }
		void Start();
		void Stop();
		void Restart() { Stop(); Start(); }

		//* Update camera data.
		//* params: t is the time in seconds since the last update
		void Update(double t);

		//* Publish camera data.
		void Publish(const Picture& pic);

	private:
		//* Binding (reference) to the Player driver.
		PlayerSRV& mPlayerDriver;

		//* Camera data structures.
		const player_devaddr_t mCameraAddr;

		//* Internal state of the interface.
		//* Active means that we're repeatedly taking and publishing new pictures.
		bool mInitialized;
		bool mActive;
		Surveyor::CameraResolution mCamRes;

		//* Frequency of updates. (in seconds)
		double mMinCycleTime;
		double mTimeElapsed;
};

#endif
