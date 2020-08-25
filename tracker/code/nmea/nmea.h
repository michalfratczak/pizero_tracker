#pragma once

#include <cstddef>
#include <string>


// keep data parsed from GGA __and__ RMC messages
// http://aprs.gids.nl/nmea/#rmc
// http://aprs.gids.nl/nmea/#gga

class nmea_t
{
public:
	// fields provided by both GGA and RMC messages:
	//
	char utc[6] = {'0', '0', '0', '0', '0', '0'}; // HHMMSS

	float lat = 0; // degree
	float lon = 0; // degree

	// GGA message fields:
	//
	float alt = 0; // meters
	int sats = 0;
	enum class fix_quality_t : int {
		kNoFix = 0,
		kAutonomous = 1,
		kDifferential = 2,
		kRtkFixed = 4,
		kRtkFloat = 5,
		kEstimated = 6
	};
	fix_quality_t fix_quality = fix_quality_t::kNoFix;


	// RMC message fields:
	//
	float speed_over_ground_mps = 0;
	float course_over_ground_deg = 0;
	enum class fix_status_t : int {
		kInvalid=0, // V
		kValid=1 // A
	};
	fix_status_t fix_status = fix_status_t::kInvalid;

	enum class nmea_msg_type_t : int {
		kUnknown = 0,
		kGGA = 1,
		kRMC = 2
	};
	nmea_msg_type_t nmea_msg_type_ = nmea_msg_type_t::kUnknown;


	std::string str() const;
	std::string json() const;

	int utc_as_seconds() const;	// convert utc HHMMSS to seconds
	bool valid() const;
};



int NMEA_checksum(const char *buff, const size_t buff_sz);

// buff is a zero terminated string
// $GNGGA,170814.00,,,,,0,00,99.99,,,,,,*73
bool NMEA_msg_checksum_ok(const char* msg, const size_t msg_sz);
bool NMEA_msg_checksum_ok(const std::string& msg);

// get full NMEA message from a buffer;
// NMEA message: $GNGGA,170814.00,,,,,0,00,99.99,,,,,,*73
// if not found, return false or empty string
// output:
//		buff+in points to $
// 		buff+out points to second char of checksum (3)

bool NMEA_get_last_msg(const char *buff, const size_t buff_sz, size_t& in, size_t& out);
std::string NMEA_get_last_msg(const char *buff, const size_t buff_sz);
std::string NMEA_get_last_msg(const std::string& msg);

// parse NMEA string for GGA __and__ RMC messages
// update o_nmea
// !! o_nmea fields that were not parsed by NMEA command will remain unmodified
bool NMEA_parse(const char *Buffer, nmea_t& o_nmea);
