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
bool NMEA_msg_checksum_ok(const char *buff);

bool NMEA_msg_checksum_ok(const std::string &msg);

float NMEA_Deg_2_Dec(float i_pos);

bool NMEA_parse(const char *Buffer, nmea_t& o_nmea);

std::ostream& operator<<(std::ostream& s, const nmea_t& nmea);
