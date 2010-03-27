#ifndef METROBOTICS_PLAYERSRV_COMMAND_DRIVESRV_H
#define METROBOTICS_PLAYERSRV_COMMAND_DRIVESRV_H

/* Forward declaration of our driver. */
#include "../PlayerSRV.h"
class PlayerSRV;

//* Factory function that allocates a new drive command.
//* Remarks: This is meant to be used in conjuction with the Command class.
CommandInterface* DriveSRV(PlayerSRV& driver,
                           int leftMotor, int rightMotor,
                           double linearVelocity, double angularVelocity);

// Command class that exectues the Surveyor's drive (motor) command.
class DriveSRV_Implementation : public CommandInterface {
	public:
		DriveSRV_Implementation(PlayerSRV& driver,
		                        int leftMotor, int rightMotor,
		                        double linearVelocity, double angularVelocity);

		// Implement the CommandInterface.
		void operator()();
		int priority() const;
		std::string id() const;

	private:
		// Binding to the driver.
		PlayerSRV& mPlayerDriver;

		// Command arguments.
		int mLeftMotor;
		int mRightMotor;
		double mLinearVelocity;
		double mAngularVelocity;
};

#endif
