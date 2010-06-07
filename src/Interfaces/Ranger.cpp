#include "Ranger.h"

using namespace std;
using namespace metrobotics;

const int NUM_IR = 4;
enum { IRFRONT = 0, IRLEFT = 1, IRBACK = 2, IRRIGHT = 3 };

Ranger::Ranger(PlayerSRV& playerDriver, player_devaddr_t addr)
:mPlayerDriver(playerDriver), mRangerAddr(addr), mActive(false), mConfState(true)
{
	// Clear our data structures.
	memset(&mRangerData, 0, sizeof(player_ranger_data_intns_t));
	memset(&mRangerConf, 0, sizeof(player_ranger_config_t));
	memset(&mRangerGeom, 0, sizeof(player_ranger_geom_t));

	// Fill in the geometry data; it never changes.
	mRangerGeom.pose.py = 0.0635; // Height of base in meters.
	mRangerGeom.size.sw = mRangerGeom.size.sl = 0.0762;
	mRangerGeom.element_poses_count = mRangerGeom.element_sizes_count = NUM_IR;
	mRangerGeom.element_poses = new player_pose3d_t[mRangerGeom.element_poses_count];
	mRangerGeom.element_sizes = new player_bbox3d_t[mRangerGeom.element_sizes_count];
	if (mRangerGeom.element_poses == 0 || mRangerGeom.element_sizes == 0) {
		throw logic_error("failed to allocate memory in ranger constructor");
	}
	for (uint32_t i = 0; i < mRangerGeom.element_poses_count; ++i) {
		memset(&mRangerGeom.element_poses[i], 0, sizeof(player_pose3d_t));
		switch (i) {
			case IRFRONT: {
				mRangerGeom.element_poses[i].pz   =  0.0381;
				mRangerGeom.element_poses[i].pyaw =  0.0000;
			} break;
			case IRLEFT: {
				mRangerGeom.element_poses[i].px   = -0.0381;
				mRangerGeom.element_poses[i].pyaw =  M_PI_2;
			} break;
			case IRBACK: {
				mRangerGeom.element_poses[i].pz   = -0.0381;
				mRangerGeom.element_poses[i].pyaw =  M_PI;
			} break;
			case IRRIGHT: {
				mRangerGeom.element_poses[i].px   =  0.0381;
				mRangerGeom.element_poses[i].pyaw = -M_PI_2;
			} break;
			default: break;
		}
	}
	for (uint32_t i = 0; i < mRangerGeom.element_sizes_count; ++i) {
		memset(&mRangerGeom.element_sizes[i], 0, sizeof(player_bbox3d_t));
		// They're all the same size.
		mRangerGeom.element_sizes[i].sl = 0.0063;
		mRangerGeom.element_sizes[i].sw = 0.0127;
		mRangerGeom.element_sizes[i].sh = 0.0127;
	}

	// Allocate space for intensity data.
	mRangerData.intensities_count = NUM_IR;
	mRangerData.intensities = new double[mRangerData.intensities_count];
	if (mRangerData.intensities == 0) {
		throw logic_error("failed to allocate memory in ranger constructor");
	}
	memset(mRangerData.intensities, 0, sizeof(double)*mRangerData.intensities_count);
}

Ranger::~Ranger()
{
	if (mRangerGeom.element_poses) {
		delete[] mRangerGeom.element_poses;
		mRangerGeom.element_poses = 0;
	}
	if (mRangerGeom.element_sizes) {
		delete[] mRangerGeom.element_sizes;
		mRangerGeom.element_sizes = 0;
	}
	if (mRangerData.intensities) {
		delete[] mRangerData.intensities;
		mRangerData.intensities = 0;
	}
}

void Ranger::ReadConfig(ConfigFile& cf, int section)
{
}

int Ranger::ProcessMessage(QueuePointer& queue, player_msghdr *msghdr, void *data)
{
	if (msghdr->type == PLAYER_MSGTYPE_REQ) {
		// Request subtypes.
		switch (msghdr->subtype) {
			case PLAYER_RANGER_REQ_GET_GEOM: {
				// Acknowledge the request by publishing the geometry data.
				mPlayerDriver.Publish(mRangerAddr, queue, PLAYER_MSGTYPE_RESP_ACK,
				                      msghdr->subtype, (void *)&mRangerGeom);
				return 0;
			} break;
			case PLAYER_RANGER_REQ_POWER: {
			} break;
			case PLAYER_RANGER_REQ_INTNS: {
				if (data) {
					mConfState = ((player_ranger_intns_config_t*)data)->state;
					if (mConfState && !mActive) {
						Start();
					} else if (mActive && !mConfState) {
						Stop();
					}
				}
				// Acknowledge the request.
				mPlayerDriver.Publish(mRangerAddr, queue, PLAYER_MSGTYPE_RESP_ACK, msghdr->subtype);
				return 0;
			} break;
			case PLAYER_RANGER_REQ_SET_CONFIG: {
			} break;
			case PLAYER_RANGER_REQ_GET_CONFIG: {
				// Acknowledge the request by publishing the config data.
				mPlayerDriver.Publish(mRangerAddr, queue, PLAYER_MSGTYPE_RESP_ACK,
				                      msghdr->subtype, (void *)&mRangerConf);
				return 0;
			} break;
			default: break;
		}
	}
	return -1;
}

void Ranger::Start()
{
	if (mConfState && !mActive && mPlayerDriver) {
		mActive = true;
		mPlayerDriver.PushCommand(BounceSRV(mPlayerDriver));
	}
}

void Ranger::Stop()
{
	mActive = false;
}

void Ranger::Publish(const IRArray& ir)
{
	if (!mPlayerDriver) {
		//* Since the driver is apparently malfunctioning, make sure not to publish
		//* anything lest we inadvertently fool the client into thinking that the 
		//* interface is fresh.
		Stop();
	} else {
		// Retrieve the data.
		mRangerData.intensities[IRFRONT] = ir.front();
		mRangerData.intensities[IRLEFT]  = ir.left();
		mRangerData.intensities[IRBACK]  = ir.back();
		mRangerData.intensities[IRRIGHT] = ir.right();
		// Publish our data.
		mPlayerDriver.Publish(mRangerAddr, PLAYER_MSGTYPE_DATA,
		                      PLAYER_RANGER_DATA_INTNS, (void *)&mRangerData);
		if (mActive && mConfState) {
			mPlayerDriver.PushCommand(BounceSRV(mPlayerDriver));
		}
	}
}
