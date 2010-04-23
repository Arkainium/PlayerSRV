#ifndef METROBOTICS_PLAYERSRV_COMMAND_TAKEPICTURESRV_H
#define METROBOTICS_PLAYERSRV_COMMAND_TAKEPICTURESRV_H

/* Forward declaration of our driver. */
#include "../PlayerSRV.h"
class PlayerSRV;

//* Factory function that allocates a new picture taking command.
//* Remarks: This is meant to be used in conjuction with the Command class.
CommandInterface* TakePictureSRV(PlayerSRV& driver);

// Command class that exectues the Surveyor's picture taking command.
class TakePictureSRV_Implementation : public CommandInterface {
	public:
		TakePictureSRV_Implementation(PlayerSRV& driver);

		// Implement the CommandInterface.
		void operator()();
		int priority() const;
		std::string id() const;

	private:
		// Binding to the driver.
		PlayerSRV& mPlayerDriver;
};

#endif
