#ifndef METROBOTICS_PLAYERSRV_COMMAND_SYNCSRV_H
#define METROBOTICS_PLAYERSRV_COMMAND_SYNCSRV_H

/* Forward declaration of our driver. */
#include "../PlayerSRV.h"
class PlayerSRV;

//* Factory function that allocates a new sync command.
//* Remarks: This is meant to be used in conjuction with the Command class.
CommandInterface* SyncSRV(PlayerSRV& driver);

// Command class that exectues the Surveyor's sync command.
class SyncSRV_Implementation : public CommandInterface {
	public:
		SyncSRV_Implementation(PlayerSRV& driver);

		// Implement the CommandInterface.
		void operator()();
		int priority() const;
		std::string id() const;

	private:
		// Binding to the driver.
		PlayerSRV& mPlayerDriver;
};

#endif
