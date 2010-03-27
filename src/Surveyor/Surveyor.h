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
#include "metrobotics.h"

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
		class InvalidSpeed {};
		class InvalidDuration {};

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
		Surveyor(const std::string& devName);

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

	private:
		// Don't allow copying or assigning of Surveyor objects!
		Surveyor(const Surveyor&);
		Surveyor& operator=(const Surveyor&);

		// Serial communication link to the physical robot.
		metrobotics::PosixSerial mDevLink;

		// Attributes that are common to all Surveyors
		// Physical dimensions in meters.
		static const double gLength;
		static const double gWidth;
		static const double gHeight;
};

#endif
