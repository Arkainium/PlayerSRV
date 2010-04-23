#ifndef METROBOTICS_PLAYERSRV_DRIVER_H
#define METROBOTICS_PLAYERSRV_DRIVER_H

#include "libplayercore/playercore.h"
#include "boost/thread/thread.hpp"
#include "boost/thread/mutex.hpp"
#include "metrobotics.h"

#include <string>
#include <list>

#include "Surveyor/Surveyor.h"
#include "Commands/Commands.h"
#include "Interfaces/Interfaces.h"

//* We need some forward declarations due to the circular nature
//* of the relationships between our classes.
class Position2D;
class Camera;

class PlayerSRV : public ThreadedDriver
{
	public:
		//* Object life cycle.
		PlayerSRV(ConfigFile *cf, int section);
		~PlayerSRV();

		//* Thread life cycle.
		int  MainSetup();
		void MainQuit();
		void Main();

		//* Message handling.
		int ProcessMessage(QueuePointer& queue, player_msghdr *msghdr, void *data);

		//* State of the driver.
		operator bool() const { return mFunctional; }

		//* Add a command to the command queue.
		void PushCommand(const Command& cmd);

		//* Accessing parts of the driver.
		//* Warning: accessing a part of the driver that requires
		//*          mutual exclusion will block the thread of execution.
		Surveyor& LockSurveyor(); void UnlockSurveyor();
		Position2D& LockPosition2D(); void UnlockPosition2D();
		Camera& LockCamera(); void UnlockCamera();

	private:
		//* Internal state of the driver.
		bool   mFunctional;
		double mMinCycleTime;

		//* Command queue.
		void clear_command_queue();
		typedef std::list<Command> CommandQueue;
		CommandQueue   mCmdQueue;
		Command        mCmdCurrent;
		boost::thread* mCmdThread;
		double         mCmdTimeout;

		//* Surveyor.
		std::string  mPortName;
		Surveyor*    mSurveyor;
		boost::mutex mSurveyorMutex;

		//* Player interfaces.
		Position2D*  mPosition2D;
		boost::mutex mPosition2DMutex;
		Camera*      mCamera;
		boost::mutex mCameraMutex;
};

//* Standard Player Protocol.
Driver* PlayerSRV_Init(ConfigFile *cf, int section);
void PlayerSRV_Register(DriverTable *table);

#endif
