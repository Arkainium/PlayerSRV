#ifndef METROBOTICS_PLAYERSRV_INTERFACE_POSITION2D_H
#define METROBOTICS_PLAYERSRV_INTERFACE_POSITION2D_H

/* Forward declaration of our driver. */
#include "../PlayerSRV.h"
class PlayerSRV;

class Position2D
{
	public:
		//* Construct a new position2d interface,
		//* and bind it to the PlayerSRV driver.
		Position2D(PlayerSRV& driver, player_devaddr_t addr);

		//* Extract any relevant information from the config file.
		void ReadConfig(ConfigFile& cf, int section);

		//* Update position data based on current velocity,
		//* and then publish the updated data to the driver.
		//* params: t is the time in seconds since the last update
		void Update(double t);

		//* Process Player commands and requests that are specific to position2d.
		int ProcessMessage(QueuePointer& queue, player_msghdr *msghdr, void *data);

		//* Address to which the interface is bound on the server.
		player_devaddr_t Address() const { return mPositionAddr; }

		//* Clear (reset) velocity and position data.
		void Reset();

		//* Update velocity (internal data only) to reflect actual (given) velocity.
		void Update(double linearVelocity, double angularVelocity);

		//* Access data.
		player_position2d_data_t Data() const { return mPositionData; }

		//* Current state of motion.
		bool Moving() const;

	private:
		//* Binding (reference) to the Player driver.
		PlayerSRV& mPlayerDriver;

		//* Position2D data structures.
		const player_devaddr_t   mPositionAddr;
		player_position2d_geom_t mPositionGeom;
		player_position2d_data_t mPositionData;

		//* Velocity mapping and interpolation.
		metrobotics::Lerp<3> mLinearVelocity;
		metrobotics::Lerp<3> mAngularVelocity;

		//* Machinery for handling velocity commands.
		int SetSpeed(player_position2d_cmd_vel_t cmd, bool fGoTo = false);
		int SetSpeed(player_pose2d_t vel, bool fGoTo = false);
		int Stop();

		//* Machinery for handling position commands.
		player_position2d_cmd_pos_t mGoToDestination;
		player_position2d_cmd_pos_t mGoToCheckpoint;
		bool mGoToState;
		int  GoTo(player_position2d_cmd_pos_t cmd);
		bool GoToAnalysis(player_position2d_cmd_pos_t& nextPos);
		void GoToUpdate();
};

#endif
