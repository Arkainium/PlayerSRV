#include "DriveSRV.h"
#include "boost/thread/thread.hpp"
using namespace std;

CommandInterface* DriveSRV(PlayerSRV& driver,
                           int leftMotor, int rightMotor,
                           double linearVelocity, double angularVelocity)
{
	return new DriveSRV_Implementation(driver,
                                       leftMotor, rightMotor,
	                                   linearVelocity, angularVelocity);
}

DriveSRV_Implementation::DriveSRV_Implementation(PlayerSRV& driver,
                                                 int leftMotor, int rightMotor,
                                                 double linearVelocity, double angularVelocity)
:mPlayerDriver(driver),
 mLeftMotor(leftMotor), mRightMotor(rightMotor),
 mLinearVelocity(linearVelocity), mAngularVelocity(angularVelocity)
{
}

int DriveSRV_Implementation::priority() const
{
	// FIXME: What priority should this be?
	// Active commands should have higher priority than passive commands.
	return 1000;
}

string DriveSRV_Implementation::id() const
{
	return string("DriveSRV");
}

void DriveSRV_Implementation::operator()()
{
	Surveyor& surveyor = mPlayerDriver.LockSurveyor();

	bool fDone = false;
	while (!fDone) {
		try {
			surveyor.drive(mLeftMotor, mRightMotor, 0);
			fDone = true;
		} catch (...) {
			fDone = false;
			// Have we had enough yet?
			try {
				boost::this_thread::interruption_point();
			} catch (boost::thread_interrupted) {
				mPlayerDriver.UnlockSurveyor();
				return;
			}
		}
	}

	// Update velocity data.
	Position2D& pos = mPlayerDriver.LockPosition2D();
	pos.Update(mLinearVelocity, mAngularVelocity);
	mPlayerDriver.UnlockPosition2D();

	mPlayerDriver.UnlockSurveyor();
}
