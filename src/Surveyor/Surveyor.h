#ifndef METROBOTICS_SURVEYOR_H
#define METROBOTICS_SURVEYOR_H
/************************************************************************/
/* This was developed by John Cummins at Brooklyn College, with         */
/* assistance from M. Q. Azhar and Howard of Surveyor.com, and          */
/* supervision from Professor Sklar.                                    */
/*                                                                      */
/* It was adopted into the Surveyor's Player driver, and is currently   */
/* maintained by Mark Manashirov from Brooklyn College and the entire   */
/* MetroBotics team at CUNY.                                            */
/*                                                                      */
/* It is released under the copyleft understanding. That is, any one is */
/* free to use, and modify, any part of it so long as it continues to   */
/* carry this notice.                                                   */
/************************************************************************/

#include <string>
#include <vector>
#include <list>
#include "metrobotics.h"
#include "Picture.h"
#include "IRArray.h"
#include "YUVRange.h"
#include "Rect.h"
#include "Blob.h"

/************************************************************************/
/* Surveyor Class                                                       */
/* The SRV-1 (ARM7 version) robot from http://www.surveyor.com          */
/* See http://www.surveyor.com/SRV_protocol_arm7.html for the official  */
/* control protocol.                                                    */
/************************************************************************/
class Surveyor
{
	public:
		// Exceptions.
		class NotResponding {};
		class OutOfSync {};
		class OutOfMemory {};
		class InvalidRect {};
		class InvalidSpeed {};
		class InvalidDuration {};
		class InvalidColorBin {};
		class InvalidScanType {};
		class InvalidResolution {};

		// Camera options.
		enum CameraResolution {
			CAMSIZE_80x64,
			CAMSIZE_160x128,
			CAMSIZE_320x240,
			CAMSIZE_640x480
		};

		// Vision-scanning options.
		enum ScanType {
			SCAN_FIRST_MATCH,
			SCAN_FIRST_MISMATCH,
			SCAN_MATCH_COUNT
		};

		/**
		 * \brief    Toggle class-wide debugging.
		 */
		static void debugging(bool);

		/**
		 * \brief    Physical length in meters.
		 */
		static double length() { return gLength; }
		/**
		 * \brief    Physical width in meters.
		 */
		static double width() { return gWidth; }
		/**
		 * \brief    Physical height in meters.
		 */
		static double height() { return gHeight; }


		/**
		 * \brief    Construct a Surveyor object.
		 * \arg      \c devName is the name of the physical device to
		 *                      which to bind the object
		 */
		Surveyor(const std::string& devName, CameraResolution res = CAMSIZE_160x128);

		/**
		 * \brief    Synchronize with the Surveyor.
		 * \details  Confirm that the communications link to the Surveyor
		 *           is functioning properly.
		 * \arg      \c maxTries is the number of synchronization attempts
		 *                       before giving up
		 */
		bool sync(unsigned int maxTries);

		/**
		 * \brief    Retrieve the Surveyor's firmware version.
		 */
		std::string getVersion();

		/**
		 * \brief    Send a drive command.
		 * \arg      \c left tread speed
		 * \arg      \c right tread speed
		 * \arg      \c duration in tens of milliseconds;
		 *                       0 (the default) drives indefinitely
		 */
		void drive(int left, int right, int duration = 0);

		/**
		 * \brief    Recall colors that are currently in the color bins.
		 * \arg      \c bin one of the Surveyor's 16 color bins.
		 */
		const YUVRange getColorBin(int bin);

		/**
		 * \brief    Sample colors by grabbing a rectangular area on the image;
		 *           the value returned is automatically save to the color bin.
		 * \arg      \c bin one of the Surveyor's 16 color bins.
		 * \arg      \c rect is the rectangular region of the object
		 */
		const YUVRange grabColorBin(int bin, const Rect& rect);

		/**
		 * \brief    Manually set colors that are stored in the color bins.
		 * \arg      \c bin one of the Surveyor's 16 color bins.
		 */
		void setColorBin(int bin, const YUVRange& color);

		/**
		 * \brief    Locate blobs in the image using the color in the given bin.
		 * \arg      \c bin one of the Surveyor's 16 color bins.
		 * \arg      \c blobs is the location where the blobs will be stored
		 */
		void getBlobs(int bin, std::list<Blob>& blobs);

		/**
		 * \brief    Uses the given color bin to scan all the columns in the image.
		 * \arg      \c bin one of the Surveyor's 16 color bins.
		 * \arg      \c cols is the location where the scan results will be stored
		 * \arg      \c type is the type of scan to perform
		 */
		void scanColumns(int bin, std::vector<int>& cols, ScanType type);

		void setResolution(CameraResolution res = CAMSIZE_160x128);
		const Picture takePicture();
		const IRArray bounceIR();

		void minTimeout(size_t s) { mMinTimeout = s; }
		void maxTimeout(size_t s) { mMaxTimeout = s; }

	private:
		// Don't allow copying or assigning of Surveyor objects!
		Surveyor(const Surveyor&);
		Surveyor& operator=(const Surveyor&);

		// Serial communication link to the physical robot.
		metrobotics::PosixSerial mDevLink;

		// Currently set image resolution.
		CameraResolution mCurrentRes;

		// Absolute minimum and maximum command time.
		size_t mMaxTimeout;
		size_t mMinTimeout;

		// Attributes that are common to all Surveyors
		// Physical dimensions in meters.
		static const double gLength;
		static const double gWidth;
		static const double gHeight;
};

#endif
