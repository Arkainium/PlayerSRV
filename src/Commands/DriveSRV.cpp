#include "DriveSRV.h"
#include "boost/thread/thread.hpp"
using namespace std;

CommandInterface* DriveSRV(PlayerSRV& driver,
                           int leftMotor, int rightMotor,
                           double linearVelocity, double angularVelocity,
                           int timeToDrive)
{
	return new DriveSRV_Implementation(driver,
                                       leftMotor, rightMotor,
	                                   linearVelocity, angularVelocity,
	                                   timeToDrive);
}

DriveSRV_Implementation::DriveSRV_Implementation(PlayerSRV& driver,
                                                 int leftMotor, int rightMotor,
                                                 double linearVelocity, double angularVelocity,
                                                 int timeToDrive)
:mPlayerDriver(driver), mTimeToDrive(timeToDrive),
 mLeftMotor(leftMotor), mRightMotor(rightMotor),
 mLinearVelocity(linearVelocity), mAngularVelocity(angularVelocity)
{
	//* Impose physical limits.
	if (mLeftMotor  < -127) mLeftMotor  = -127;
	if (mLeftMotor  >  127) mLeftMotor  =  127;
	if (mRightMotor < -127) mRightMotor = -127;
	if (mRightMotor >  127) mRightMotor =  127;
	if (mTimeToDrive <   0) mTimeToDrive = 0;
	if (mTimeToDrive > 255) mTimeToDrive = 255;
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
			surveyor.drive(mLeftMotor, mRightMotor, mTimeToDrive);
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

	// Update position2d data.
	Position2D& pos = mPlayerDriver.LockPosition2D();
	pos.Update(mLinearVelocity, mAngularVelocity,
	           (mTimeToDrive <= 0 ? -1 : mTimeToDrive/100.0));
	mPlayerDriver.UnlockPosition2D();

	mPlayerDriver.UnlockSurveyor();
}
