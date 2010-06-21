#ifndef METROBOTICS_PLAYERSRV_INTERFACE_BLOBFINDER_H
#define METROBOTICS_PLAYERSRV_INTERFACE_BLOBFINDER_H

/* Forward declaration of our driver. */
#include "../PlayerSRV.h"
class PlayerSRV;

#include <list>
#include <map>

class BlobFinder
{
	public:
		//* Surveyor-specific attributes.
		static const int MAX_BLOBS = 16;
		static const int BLOB_WINDOW_WIDTH  = 80;
		static const int BLOB_WINDOW_HEIGHT = 64;

		//* Construct a new blobfinder interface,
		//* and bind it to the PlayerSRV driver.
		BlobFinder(PlayerSRV& driver, player_devaddr_t addr);

		//* Extract any relevant information from the config file.
		void ReadConfig(ConfigFile& cf, int section);

		//* Process Player requests that are specific to blobfinder.
		int ProcessMessage(QueuePointer& queue, player_msghdr *msghdr, void *data);

		//* Address to which the interface is bound on the server.
		player_devaddr_t Address() const { return mBlobFinderAddr; }

		//* State management.
		bool Active() const { return mActive; }
		void Start();
		void Stop();
		void Restart() { Stop(); Start(); }

		//* Update blobfinder data.
		//* params: t is the time in seconds since the last update
		void Update(double t);

		//* Publish blobfinder data.
		void Publish(int channel, const std::list<Blob>& blobs);

	private:
		//* Binding (reference) to the Player driver.
		PlayerSRV& mPlayerDriver;

		//* BlobFinder data structures.
		const player_devaddr_t   mBlobFinderAddr;
		player_blobfinder_data_t mBlobFinderData;
		player_blobfinder_blob_t mBlobFinderBlobs[BlobFinder::MAX_BLOBS];
		player_blobfinder_color_config_t mBlobFinderConfig[BlobFinder::MAX_BLOBS];
		uint32_t mBlobFinderColors[BlobFinder::MAX_BLOBS];

		//* Internal state of the interface.
		bool     mInitialized[BlobFinder::MAX_BLOBS];
		bool     mActive;
		uint32_t mActiveChannel;

		//* Frequency of updates. (in seconds)
		double mMinCycleTime;
		double mTimeElapsed;
};

#endif
