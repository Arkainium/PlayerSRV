#include "Position2D.h"
#include <cmath>

using namespace std;
using namespace metrobotics;

// Because who doesn't love π?
static const double TWOPI = 2*3.141592654;
static const double PI    =   3.141592654;

// Express an angle between -π and π radians.
static double __normalizeAngle(double theta)
{
	theta = fmod(theta, TWOPI);
	if (theta > PI) {
		theta -= TWOPI;
	} else if (theta < -PI) {
		theta += TWOPI;
	}
	return theta;
}

Position2D::Position2D(PlayerSRV& playerDriver, player_devaddr_t addr)
:mPlayerDriver(playerDriver), mPositionAddr(addr)
{
	// Clear out internal data.
	Reset();
}

void Position2D::ReadConfig(ConfigFile& cf, int section)
{
	// Extract velocity data.
	int points = 0;
	// Linear velocity.
	mLinearVelocity.clear();
	points = cf.GetTupleCount(section, "linear_velocity");
	if (points > 0) {
		for (int i = 0, j = 0; i < points; i += j) {
			RealVector3 v;
			// Each point has three components.
			for (j = 0; i+j < points && j < 3; ++j) {
				v[j] = cf.ReadTupleLength(section, "linear_velocity", i+j, 0);
			}
			// Complete point?
			if (j == 3) {
				mLinearVelocity.insert(v);
			}
		}
	} else {
		PLAYER_WARN("Position2D: no linear velocity data");
	}
	// Angular velocity.
	mAngularVelocity.clear();
	points = cf.GetTupleCount(section, "angular_velocity");
	if (points > 0) {
		for (int i = 0, j = 0; i < points; i += j) {
			RealVector3 v;
			// Each point has three components.
			for (j = 0; i+j < points && j < 3; ++j) {
				v[j] = cf.ReadTupleLength(section, "angular_velocity", i+j, 0);
			}
			// Complete point?
			if (j == 3) {
				mAngularVelocity.insert(v);
			}
		}
	} else {
		PLAYER_WARN("Position2D: no angular velocity data");
	}
}

void Position2D::Reset()
{
	// Clear the data structures.
	memset(&mPositionData, 0, sizeof(player_position2d_data_t));
	memset(&mPositionGeom, 0, sizeof(player_position2d_geom_t));

	// Fill in the geometry data.
	// Physical dimensions never change.
	mPositionGeom.size.sl = Surveyor::length();
	mPositionGeom.size.sw = Surveyor::width();
	mPositionGeom.size.sh = Surveyor::height();
	// Pose with respect to the base never changes either.
	mPositionGeom.pose.px     = 0;
	mPositionGeom.pose.py     = 0;
	mPositionGeom.pose.pz     = 0;
	mPositionGeom.pose.proll  = 0;
	mPositionGeom.pose.ppitch = 0;
	mPositionGeom.pose.pyaw   = 0;

	// Clear GoTo state.
	mGoToState = false;
}

void Position2D::Update(double linearVelocity, double angularVelocity)
{
	mPositionData.vel.px = linearVelocity;
	mPositionData.vel.px = 0;
	mPositionData.vel.pa = angularVelocity;
}

bool Position2D::Moving() const
{
	return mPositionData.vel.px || mPositionData.vel.py || mPositionData.vel.pa;
}

void Position2D::Update(double t)
{
	if (!mPlayerDriver || t < 0) {
		//* Since the driver is apparently malfunctioning, make sure not to publish
		//* anything lest we inadvertently fool the client into thinking that the 
		//* interface is fresh.
	} else if (!Moving() && mGoToState) {
		//* We're not currently moving, but we should be because
		//* we've got places to go and people to see!
		if (GoToAnalysis(mGoToCheckpoint)) {
			// Go to the next checkpoint.
			SetSpeed(mGoToCheckpoint.vel, mGoToState);
		} else {
			// We've arrived at our destination!
			mGoToState = false;
		}
	} else if (Moving() && t > 0) {
		//* Update our odometry.

		// Compute our new angle.
		const double da = mPositionData.vel.pa * t;
		mPositionData.pos.pa = __normalizeAngle(mPositionData.pos.pa + da);

		// Compute our new position.
		const double dx = mPositionData.vel.px * t;
		mPositionData.pos.px += dx * cos(mPositionData.pos.pa);
		mPositionData.pos.py += dx * sin(mPositionData.pos.pa);

		// Are we currently handling a GoTo (position) command?
		if (mGoToState) {
			GoToUpdate();
		}

		// Publish the updated data.
		mPlayerDriver.Publish(mPositionAddr, PLAYER_MSGTYPE_DATA,
		                      PLAYER_POSITION2D_DATA_STATE, (void *)&mPositionData);
	}
}

int Position2D::ProcessMessage(QueuePointer& queue, player_msghdr *msghdr, void *data)
{
	if (msghdr->type == PLAYER_MSGTYPE_REQ) {
		// Request subtypes.
		switch (msghdr->subtype) {
			case PLAYER_POSITION2D_REQ_GET_GEOM: {
				// Acknowledge the request by publishing the geometry data.
				mPlayerDriver.Publish(mPositionAddr, queue, PLAYER_MSGTYPE_RESP_ACK,
				                      msghdr->subtype, (void *)&mPositionGeom);
				return 0;
			} break;
			case PLAYER_POSITION2D_REQ_MOTOR_POWER: {
			} break;
			case PLAYER_POSITION2D_REQ_VELOCITY_MODE: {
			} break;
			case PLAYER_POSITION2D_REQ_POSITION_MODE: {
			} break;
			case PLAYER_POSITION2D_REQ_SET_ODOM: {
				// Apply the odometry values that were sent along with the message.
				mPositionData.pos = ((player_position2d_set_odom_req_t*)data)->pose;
				// Acknowledge the request by publishing the new odometry.
				mPlayerDriver.Publish(mPositionAddr, queue, PLAYER_MSGTYPE_RESP_ACK,
				                      msghdr->subtype, (void *)&mPositionData.pos);
				return 0;
			} break;
			case PLAYER_POSITION2D_REQ_RESET_ODOM: {
			} break;
			case PLAYER_POSITION2D_REQ_SPEED_PID: {
			} break;
			case PLAYER_POSITION2D_REQ_POSITION_PID: {
			} break;
			case PLAYER_POSITION2D_REQ_SPEED_PROF: {
			} break;
			default: break;
		}
	} else if (msghdr->type == PLAYER_MSGTYPE_CMD) {
		// Command subtypes.
		switch (msghdr->subtype) {
			case PLAYER_POSITION2D_CMD_VEL: {
				// Delegate the command to another member function.
				return SetSpeed(*((player_position2d_cmd_vel_t*)data), false);
			} break;
			case PLAYER_POSITION2D_CMD_POS: {
				// Delegate the command to another member function.
				return GoTo(*((player_position2d_cmd_pos_t*)data));
			} break;
			case PLAYER_POSITION2D_CMD_CAR: {
			} break;
			case PLAYER_POSITION2D_CMD_VEL_HEAD: {
			} break;
			default: break;
		}
	}
	return -1;
}

int Position2D::SetSpeed(player_position2d_cmd_vel_t cmd, bool fGoTo)
{
	// Make sure that we're in a functional state.
	if (!mPlayerDriver) {
		return -1;
	}

	// Is this command part of a GoTo sequence?
	if (!fGoTo) {
		// No: override any current GoTo (position) command.
		mGoToState = false;
	}

	//* Compute which values to send to the left motor and right motor.
	int leftMotor = 0, rightMotor = 0;
	// FIXME: keeping turning and driving mutually exclusive for now to keep things simple.
	// Since the Surveyor is nonholonomic, ignore the y-axis velocity completely.
	// Case 1: Turning has precedence.
	if (cmd.vel.pa) {
		try {
			RealVector3 v = mAngularVelocity.truncate(cmd.vel.pa);
			cmd.vel.px = 0;
			cmd.vel.pa = v[0];
			leftMotor  = static_cast<int>(v[1]);
			rightMotor = static_cast<int>(v[2]);
		} catch (...) {
			if (mAngularVelocity.empty()) {
				PLAYER_ERROR("Position2D: turning is disabled because there is no angular velocity data");
			} else {
				PLAYER_ERROR("Position2D: failed to compute angular velocity");
			}
			cmd.vel.px = cmd.vel.pa = leftMotor = rightMotor = 0;
		}
	// Case 2: Straight driving.
	} else if (cmd.vel.px) {
		try {
			RealVector3 v = mLinearVelocity.truncate(cmd.vel.px);
			cmd.vel.px = v[0];
			cmd.vel.pa = 0;
			leftMotor  = static_cast<int>(v[1]);
			rightMotor = static_cast<int>(v[2]);
		} catch (...) {
			if (mLinearVelocity.empty()) {
				PLAYER_ERROR("Position2D: driving is disabled because there is no linear velocity data");
			} else {
				PLAYER_ERROR("Position2D: failed to compute linear velocity");
			}
			cmd.vel.px = cmd.vel.pa = leftMotor = rightMotor = 0;
		}
	// Case 3: Stop.
	} else {
		cmd.vel.px = cmd.vel.pa = leftMotor = rightMotor = 0;
	}

	//* By now we should have appropriate values for the left motor and right motor.
	//* Use these values to construct a new command and pass it to the driver.
	mPlayerDriver.PushCommand(DriveSRV(mPlayerDriver,
	                                   leftMotor, rightMotor,
	                                   cmd.vel.px, cmd.vel.pa));
	return 0;
}

int Position2D::SetSpeed(player_pose2d_t vel, bool fGoTo)
{
	player_position2d_cmd_vel_t cmd;
	memset(&cmd, 0, sizeof(player_position2d_cmd_vel_t));
	cmd.vel = vel;
	return SetSpeed(cmd, fGoTo);
}

int Position2D::Stop()
{
	if (Moving()) {
		player_position2d_cmd_vel_t stop;
		memset(&stop, 0, sizeof(player_position2d_cmd_vel_t));
		return SetSpeed(stop, mGoToState);
	}
	return 0;
}

int Position2D::GoTo(player_position2d_cmd_pos_t cmd)
{
	// Make sure that we're in a functional state.
	if (!mPlayerDriver) {
		return -1;
	}

	// Override any previous GoTo (position) command.
	mGoToDestination = cmd;
	mGoToState = true;

	//* In order for GoTo processing to commence on the next update,
	//* we have to make sure that we're not already moving.
	return Stop();
}

bool Position2D::GoToAnalysis(player_position2d_cmd_pos_t& nextPos)
{
	// Allow a certain degree of error when comparing angles.
	static RealEquality eqAngles((1.0/180.0)*PI);

	// Allow a certain margin of error when comparing distances.
	static RealEquality eqDistances(0.01);

	// Allow a certain margin of error when comparing velocities.
	static RealEquality eqVelocities(0.01);

	// Make sure that we're currently handling a goto command.
	if (!mGoToState) {
		return false;
	}

	// Compute the offsets of the new position from the current position.
	// Think of this as a coordinate system that is local to our robot.
	double dx = mGoToDestination.pos.px - mPositionData.pos.px;
	double dy = mGoToDestination.pos.py - mPositionData.pos.py;

	// Compute the distance of the line segment formed by the two positions.
	// Think of this as the radius of the circle centered at our robot.
	double distance = sqrt(pow(dx, 2) + pow(dy, 2));
	if (eqDistances(distance, 0.0)) {
		distance = 0.0;
	}

	// Compute the direction (angle) of the new position relative to the current position.
	// That is, how much and in which direction must we turn in order to face the new position.
	double theta = 0.0;
	if (!eqDistances(distance, 0.0)) {
		theta = __normalizeAngle(acos(dx / distance) - mPositionData.pos.pa);
	} else if (fabs(mGoToDestination.pos.pa) <= TWOPI) {
		theta = __normalizeAngle(mGoToDestination.pos.pa - mPositionData.pos.pa);
	}
	if (eqAngles(theta, 0.0)) {
		theta = 0.0;
	}

	//* Decompose the final destination into a sequence of checkpoints.
	if (!eqAngles(theta, 0.0)) {
		// Command: turn towards the new position.
		memset(&nextPos, 0, sizeof(player_position2d_cmd_pos_t));
		nextPos.pos.pa = mPositionData.pos.pa + theta;
		// Determine how fast we should travel to the new position.
		nextPos.vel.pa = theta > 0.0 ?  fabs(mGoToDestination.vel.pa)
		                             : -fabs(mGoToDestination.vel.pa);
		if (eqVelocities(nextPos.vel.pa, 0.0)) {
			nextPos.vel.pa = theta > 0.0 ? mAngularVelocity.max()[0]
			                             : mAngularVelocity.min()[0];
		}
		return true;
	}
	if (!eqDistances(distance, 0.0)) {
		// Command: drive to the new position.
		memset(&nextPos, 0, sizeof(player_position2d_cmd_pos_t));
		nextPos.pos.px = mGoToDestination.pos.px;
		nextPos.pos.py = mGoToDestination.pos.py;
		// Determine how fast we should travel to the new position.
		nextPos.vel.px = distance > 0.0 ?  fabs(mGoToDestination.vel.px)
		                                : -fabs(mGoToDestination.vel.px);
		if (eqVelocities(nextPos.vel.px, 0.0)) {
			nextPos.vel.px = distance > 0.0 ? mLinearVelocity.max()[0]
			                                : mLinearVelocity.min()[0];
		}
		return true;
	}

	return false;
}

void Position2D::GoToUpdate()
{
	// Are we currently handling a GoTo (position) command?
	if (mGoToState && Moving()) {
		// Analyze our current position with respect to the final destination.
		player_position2d_cmd_pos_t next;
		if (GoToAnalysis(next)) {
			// Are we turning?
			if (mPositionData.vel.pa) {
				// Should we still be turning?
				if ((next.vel.pa == 0) ||
				    ((next.vel.pa > 0) != (mPositionData.vel.pa > 0))) {
					Stop();
				}
			// Are we driving?
			} else if (mPositionData.vel.px) {
				// Should we still be driving?
				if ((next.vel.px == 0) ||
				    ((next.vel.px > 0) != (mPositionData.vel.px > 0))) {
					Stop();
				}
			}
		} else {
			// We're here!
			Stop();
		}
	}
}
