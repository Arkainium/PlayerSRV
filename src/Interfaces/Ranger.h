#ifndef METROBOTICS_PLAYERSRV_INTERFACE_RANGER_H
#define METROBOTICS_PLAYERSRV_INTERFACE_RANGER_H

/* Forward declaration of our driver. */
#include "../PlayerSRV.h"
class PlayerSRV;

class Ranger
{
	public:
		//* SRV-1 has four infrared sensors.
		static const int NUM_IR;
		enum { IRFRONT = 0, IRLEFT = 1, IRBACK = 2, IRRIGHT = 3 };

		//* Construct a new ranger interface,
		//* and bind it to the PlayerSRV driver.
		Ranger(PlayerSRV& driver, player_devaddr_t addr);
		~Ranger();

		//* Extract any relevant information from the config file.
		void ReadConfig(ConfigFile& cf, int section);

		//* Process Player requests that are specific to ranger.
		int ProcessMessage(QueuePointer& queue, player_msghdr *msghdr, void *data);

		//* Address to which the interface is bound on the server.
		player_devaddr_t Address() const { return mRangerAddr; }

		//* State management.
		bool Active() const { return mActive; }
		void Start();
		void Stop();
		void Restart() { Stop(); Start(); }

		//* Update ranger data.
		//* params: t is the time in seconds since the last update
		void Update(double t);

		//* Publish ranger data.
		void Publish(const IRArray& ir);

	private:
		//* Binding (reference) to the Player driver.
		PlayerSRV& mPlayerDriver;

		//* Ranger data structures.
		const player_devaddr_t mRangerAddr;
		player_ranger_geom_t   mRangerGeom;
		player_ranger_config_t mRangerConf;
		player_ranger_data_intns_t mRangerData;

		//* Internal state of the interface.
		//* Active means that we're currently bouncing infrared.
		bool mActive; // current state
		bool mConfState; // enable/disable specified by client

		//* Frequency of updates. (in seconds)
		double mMinCycleTime;
		double mTimeElapsed;
};

#endif
