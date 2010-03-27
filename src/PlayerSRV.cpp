#include "PlayerSRV.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace std;
using namespace metrobotics;

//* Driver-specific initialization: performed once upon startup of the server.
PlayerSRV::PlayerSRV(ConfigFile *cf, int section)
: ThreadedDriver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN),
  mCmdThread(0), mSurveyor(0), mPosition2D(0)
{
	//* Configure the driver by reading the configuration file.

	//* Extract the port name from the configuration file.
	mPortName = cf->ReadString(section, "port", "");
	if (mPortName.size() == 0) {
		PLAYER_WARN("PlayerSRV: port name is missing");
	}

	//* Initialize the minimum cycle time of the main thread.
	mMinCycleTime = cf->ReadFloat(section, "min_cycle_time", -1);
	if (mMinCycleTime < 0.0) {
		mMinCycleTime = 0.01; // seconds
	}

	//* Initialize the maximum time allotted for an executing command.
	mCmdTimeout = cf->ReadFloat(section, "command_timeout", -1);
	if (mCmdTimeout < 1.0) {
		mCmdTimeout = 10.0; // seconds
	}

	//* Add the interfaces that we're providing.
	player_devaddr_t tempAddr;

	//* Are we providing a position2D interface?
	if ((cf->ReadDeviceAddr(&tempAddr, section, "provides", PLAYER_POSITION2D_CODE, -1, 0) == 0) &&
	    (AddInterface(tempAddr) == 0)) {
		mPosition2DMutex.lock();
		mPosition2D = new Position2D(*this, tempAddr);
		if (mPosition2D == 0) {
			PLAYER_ERROR("PlayerSRV: failed to allocate memory for position2d");
		} else {
			mPosition2D->ReadConfig(*cf, section);
		}
		mPosition2DMutex.unlock();
	} else {
		PLAYER_WARN("PlayerSRV: not providing a position2D interface");
	}
}

//* Driver-specific shutdown: performed once upon shutdown of the server.
PlayerSRV::~PlayerSRV()
{
	//* Free any memory used up by our interfaces.
	mPosition2DMutex.lock();
	if (mPosition2D) {
		delete mPosition2D;
		mPosition2D = 0;
	}
	mPosition2DMutex.unlock();
}

//* Device-specific initialization: called everytime the driver goes from
//* having no subscribers to receiving the first subscription.
int PlayerSRV::MainSetup()
{
	PLAYER_MSG0(1, "PlayerSRV: activating the driver");

	//* Need a port name to connect to the Surveyor.
	if (mPortName.size() == 0) {
		PLAYER_ERROR("PlayerSRV: a valid port name is required");
		return -1;
	}

	//* This step is crucial!
	//* If we can't construct a Surveyor object
	//* then the driver is basically inoperable.
	try {
		mSurveyorMutex.lock();
		mSurveyor = new Surveyor(mPortName);
		if (mSurveyor == 0) {
			PLAYER_ERROR("PlayerSRV: failed to allocate memory for the Surveyor");
			mSurveyorMutex.unlock();
			return -1;
		}
		mSurveyorMutex.unlock();
	} catch (Serial::ConnectionFailure) {
		PLAYER_ERROR("PlayerSRV: failed to open port");
		mSurveyorMutex.unlock();
		return -1;
	} catch (...) {
		PLAYER_ERROR("PlayerSRV: unknown failure");
		mSurveyorMutex.unlock();
		return -1;
	}

	//* Reset our interfaces.
	mPosition2DMutex.lock();
	if (mPosition2D) {
		mPosition2D->Reset();
	}
	mPosition2DMutex.unlock();

	//* Clear the command queue, and stop any pending commands.
	clear_command_queue();

	//* We're fully activated now.
	//* Assume a functional state.
	mFunctional = true;
	return 0;
}

//* Device-specific shutdown: called whenever the driver goes from a state in which clients
//* are subscribed to a device to a state in which there are no longer any clients subscribed.
void PlayerSRV::MainQuit()
{
	PLAYER_MSG0(1, "PlayerSRV: deactivating the driver");

	//* Clear the command queue, and stop any pending commands.
	clear_command_queue();

	//* We no longer need the Surveyor object, so release it.
	mSurveyorMutex.lock();
	if (mSurveyor) {
		delete mSurveyor;
		mSurveyor = 0;
	}
	mSurveyorMutex.unlock();
}

void PlayerSRV::Main()
{
	PLAYER_MSG0(1, "PlayerSRV: driver is up and running");

	//* Allocate our timers before entering the main loop.
	PosixTimer tCycle;    // duration of each cycle
	PosixTimer tCommand;  // duration of the current command
	PosixTimer tPosition; // time in between position2d updates

	while (1) {
		//* Time the duration of the cycle.
		tCycle.start();

		//* Check for stop condition.
		pthread_testcancel();

		//* Are we currently executing a command?
		if (mCmdThread) {
			//* Has the command finished executing yet,
			//* or has it exceeded its allotted time?
			if(!mCmdThread->joinable() ||
			   mCmdThread->timed_join(boost::posix_time::milliseconds(1)) ||
			   tCommand.elapsed() > mCmdTimeout) {
				if (mCmdCurrent != mCmdQueue.end()) {
					// Did the command timeout?
					if (tCommand.elapsed() > mCmdTimeout) {
						// Commands that timeout are a serious problem.
						PLAYER_WARN1("PlayerSRV: %s command timed out", mCmdCurrent->id().c_str());
						// Put the driver into an erroneous state.
						mFunctional = false;
						// Try to fix it.
						PushCommand(SyncSRV(*this));
					} else {
						// The command finished on time.
						// That means the driver is functioning properly.
						mFunctional = true;
					}
					// Remove the command from the queue.
					mCmdQueue.erase(mCmdCurrent);
					mCmdCurrent = mCmdQueue.end();
				}
				// Kill the thread.
				mCmdThread->interrupt();
				delete mCmdThread;
				mCmdThread = 0;
			}
		}

		//* Are we ready to execute the next command?
		if (mCmdThread == 0 && !mCmdQueue.empty()) {
			// Pick the command with the highest priority.
			mCmdCurrent = max_element(mCmdQueue.begin(), mCmdQueue.end());
			// Execute it.
			mCmdThread = new boost::thread(*mCmdCurrent);
			// Start the clock.
			tCommand.start();
		}

		//* Process incoming messages.
		ProcessMessages();

		//* Update our interfaces.
		mPosition2DMutex.lock();
		if (mPosition2D) {
			mPosition2D->Update(tPosition.elapsed());
			tPosition.start();
		}
		mPosition2DMutex.unlock();

		//* Don't exceed the minimum cycle time.
		double timeLeft = mMinCycleTime - tCycle.elapsed();
		if (timeLeft > 0) {
			usleep(timeLeft*1000000);
		} else {
			usleep(1);
		}
	}
}

int PlayerSRV::ProcessMessage(QueuePointer& queue, player_msghdr *msghdr, void *data)
{
	int ret = -1; // default return value

	//* Match messages by passing them to their respective interface.

	mPosition2DMutex.lock();
	if (mPosition2D) {
		if (Message::MatchMessage(msghdr, -1, -1, mPosition2D->Address())) {
			ret = mPosition2D->ProcessMessage(queue, msghdr, data);
		}
	}
	mPosition2DMutex.unlock();

	return ret;
}

void PlayerSRV::PushCommand(const Command& cmd)
{
	//* Override any existing commands of this type whether the
	//* command is pending on the queue or already executing.
	if (mCmdThread && mCmdCurrent != mCmdQueue.end() && *mCmdCurrent == cmd) {
		mCmdThread->interrupt();
		mCmdCurrent = mCmdQueue.end();
	}
	remove(mCmdQueue.begin(), mCmdQueue.end(), cmd);
	mCmdQueue.push_back(cmd);
}

void PlayerSRV::clear_command_queue()
{
	//* Clear the command queue, and stop any pending commands.
	mCmdQueue.clear();
	mCmdCurrent = mCmdQueue.end();
	if (mCmdThread) {
		mCmdThread->interrupt();
		delete mCmdThread;
		mCmdThread = 0;
	}
}

Surveyor& PlayerSRV::LockSurveyor()
{
	mSurveyorMutex.lock();
	if (mSurveyor == 0) {
		throw logic_error("PlayerSRV::LockSurveyor(): null pointer exception");
	} else {
		return *mSurveyor;
	}
}

void PlayerSRV::UnlockSurveyor()
{
	mSurveyorMutex.unlock();
}

Position2D& PlayerSRV::LockPosition2D()
{
	mPosition2DMutex.lock();
	if (mPosition2D == 0) {
		throw logic_error("PlayerSRV::LockPosition2D(): null pointer exception");
	} else {
		return *mPosition2D;
	}
}

void PlayerSRV::UnlockPosition2D()
{
	mPosition2DMutex.unlock();
}

// Driver Class Factory
Driver* PlayerSRV_Init(ConfigFile *cf, int section)
{
	return ((Driver *) (new PlayerSRV(cf, section)));
}

// Driver Register Hook
void PlayerSRV_Register(DriverTable *table)
{
	table->AddDriver("PlayerSRV", PlayerSRV_Init);
}

/* NOTE: the extern is required to avoid C++ name mangling. */
extern "C"
{
	// Driver Load Hook
	int player_driver_init(DriverTable *table)
	{
		PlayerSRV_Register(table);
		return 0;
	}
}
