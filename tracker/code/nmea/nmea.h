#pragma once

#include <cstddef>
#include <string>


class nmea_t
{
public:
	int utc = 0;

	float lat = 0; // degree
	float lon = 0; // degree
	float alt = 0; // meters

	float speed_over_ground_mps = 0;
	float course_over_ground_deg = 0;

	int sats = 0;

	enum class fix_status_t {
		kInvalid=0, // V
		kValid=1 // A
	};
	fix_status_t fix_status = fix_status_t::kInvalid;

	enum class fix_quality_t {
		kNoFix = 0,
		kAutonomous = 1,
		kDifferential = 2,
		kRtkFixed = 4,
		kRtkFloat = 5,
		kEstimated = 6
	};
	fix_quality_t fix_quality = fix_quality_t::kNoFix;
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

float NMEA_Deg_2_Dec(float i_pos);

bool NMEA_parse(const char *Buffer, nmea_t& o_nmea);

std::ostream& operator<<(std::ostream& s, const nmea_t& nmea);
