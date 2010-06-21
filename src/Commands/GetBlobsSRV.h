#ifndef METROBOTICS_PLAYERSRV_COMMAND_GETBLOBSSRV_H
#define METROBOTICS_PLAYERSRV_COMMAND_GETBLOBSSRV_H

/* Forward declaration of our driver. */
#include "../PlayerSRV.h"
class PlayerSRV;

//* Factory function that allocates a new blob command.
//* Remarks: This is meant to be used in conjuction with the Command class.
CommandInterface* GetBlobsSRV(PlayerSRV& driver, int channel = 0);

// Command class that exectues the Surveyor's picture taking command.
class GetBlobsSRV_Implementation : public CommandInterface {
	public:
		GetBlobsSRV_Implementation(PlayerSRV& driver, int channel = 0);

		// Implement the CommandInterface.
		void operator()();
		int priority() const;
		std::string id() const;

	private:
		// Binding to the driver.
		PlayerSRV& mPlayerDriver;

		// Command arguments.
		int mChannel;
};

#endif
