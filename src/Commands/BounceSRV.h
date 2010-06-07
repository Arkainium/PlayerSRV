#ifndef METROBOTICS_PLAYERSRV_COMMAND_BOUNCESRV_H
#define METROBOTICS_PLAYERSRV_COMMAND_BOUNCESRV_H

/* Forward declaration of our driver. */
#include "../PlayerSRV.h"
class PlayerSRV;

//* Factory function that allocates a new sync command.
//* Remarks: This is meant to be used in conjuction with the Command class.
CommandInterface* BounceSRV(PlayerSRV& driver);

// Command class that exectues the Surveyor's infrared command.
class BounceSRV_Implementation : public CommandInterface {
	public:
		BounceSRV_Implementation(PlayerSRV& driver);

		// Implement the CommandInterface.
		void operator()();
		int priority() const;
		std::string id() const;

	private:
		// Binding to the driver.
		PlayerSRV& mPlayerDriver;
};

#endif
