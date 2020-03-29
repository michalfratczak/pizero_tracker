#include "nmea.h"

#include <math.h>
#include <cstring>
#include <iostream>

int NMEA_checksum(const char *buff, const size_t buff_sz)
{
	int c = 0;
	for (size_t i = 0; i < buff_sz; ++i)
		c ^= *buff++;
	return c;
}

// buff is a zero terminated string
bool NMEA_msg_checksum_ok(const char *buff)
{
	size_t max_scan = 100;
	const char *buff_dollar = buff;
	while (max_scan-- && *buff_dollar != '$' && *buff_dollar != '\0')
		buff_dollar++;
	if (!max_scan || *buff_dollar == '\0')
		return 0;

	max_scan = 100;
	const char *buff_star = buff_dollar + 1;
	while (max_scan-- && *buff_star != '*' && *buff_star != '\0')
		buff_star++;
	if (!max_scan || *buff_star == '\0')
		return 0;

	// std::cout<<*buff_dollar<<" "<<*buff_star<<std::endl;

	int chck = NMEA_checksum(buff_dollar + 1, buff_star - buff_dollar - 1);
	char chck_str[2];
	sprintf(chck_str, "%02X", chck);

	bool checksum_is_equal =
		chck_str[0] == *(buff_star + 1) && chck_str[1] == *(buff_star + 2);

	return checksum_is_equal;
}

bool NMEA_msg_checksum_ok(const std::string &msg)
{
	return NMEA_msg_checksum_ok(msg.c_str());
}

float NMEA_Deg_2_Dec(float i_pos)
{
	float degs = trunc(i_pos / 100);
	float mins = i_pos - 100.0f * degs;
	float res = degs + mins / 60.0f;
	return res;
}


bool NMEA_parse(const char *Buffer, nmea_t& o_nmea)
{
	using namespace std;

	// cout << "??? " << Buffer << endl;

	float utc = 0;
	int date = 0;

	float lat = 0;
	char ns = 0;
	float lon = 0;
	char ew = 0;

	float alt = 0;
	char alt_units = 0;

	// Possible values for status:
	//		V = Data invalid
	//		A = Data valid
	char status = 0;

	// Possible values for quality:
	//		0 = No fix
	//		1 = Autonomous GNSS fix
	//		2 = Differential GNSS fix
	//		4 = RTK fixed
	//		5 = RTK float
	//		6 = Estimated/Dead reckoning fix
	int quality = 0;

	int sats = 0;
	float hdop = 0; // Horizontal Dilution of Precision
	float speedOG = 0;
	float courseOG = 0;
	char speedstring[16];
	char coursestring[16];

	if (strncmp(Buffer + 3, "GGA", 3) == 0) // Global positioning system fix data
	{
		int scanned_positions =
			sscanf(Buffer + 7, "%f,%f,%c,%f,%c,%d,%d,%f,%f,%c", &
				utc, &lat, &ns, &lon, &ew, &quality, &sats, &hdop, &alt, &alt_units);
		if(scanned_positions == 10)
		{
			lat = NMEA_Deg_2_Dec(lat);
			if(ns == 'S')
				lat = -lat;

			lon = NMEA_Deg_2_Dec(lon);
			if(ew == 'W')
				lon = -lon;


			o_nmea.utc = utc;
			o_nmea.lat = lat;
			o_nmea.lon = lon;
			o_nmea.alt = alt;
			o_nmea.sats = sats;

			switch(quality)
			{
				case 0:
					o_nmea.fix_quality = nmea_t::fix_quality_t::kNoFix;
					break;
				case 1:
					o_nmea.fix_quality = nmea_t::fix_quality_t::kAutonomous;
					break;
				case 2:
					o_nmea.fix_quality = nmea_t::fix_quality_t::kDifferential;
					break;
				case 4:
					o_nmea.fix_quality = nmea_t::fix_quality_t::kRtkFixed;
					break;
				case 5:
					o_nmea.fix_quality = nmea_t::fix_quality_t::kRtkFloat;
					break;
				case 6:
					o_nmea.fix_quality = nmea_t::fix_quality_t::kEstimated;
					break;
			}
			return true;
		}
		else
		{
			// cerr<<"GGA scanf err"<<endl;
			return false;
		}

	} // GGA
	else if (strncmp(Buffer+3, "RMC", 3) == 0)
	{
		speedstring[0] = '\0';
		coursestring[0] = '\0';
		int scanned_positions =
			sscanf(Buffer+7, "%f,%c,%f,%c,%f,%c,%[^','],%[^','],%d",
				&utc, &status, &lat, &ns, &lon, &ew, speedstring, coursestring, &date);
		if(scanned_positions == 7)
		{
			speedOG = atof(speedstring);
			courseOG = atof(coursestring);

			lat = NMEA_Deg_2_Dec(lat);
			if(ns == 'S')
				lat = -lat;

			lon = NMEA_Deg_2_Dec(lon);
			if(ew == 'W')
				lon = -lon;

			o_nmea.utc = utc;
			o_nmea.lat = lat;
			o_nmea.lon = lon;
			o_nmea.alt = alt;
			o_nmea.sats = sats;
			o_nmea.speed_over_ground_mps = speedOG;
			o_nmea.course_over_ground_deg = courseOG;

			if(status == 'A')
				o_nmea.fix_status = nmea_t::fix_status_t::kValid;
			else if(status == 'V')
				o_nmea.fix_status = nmea_t::fix_status_t::kInvalid;

			return true;
		}
		else
		{
			// cerr<<"RMC scanf err"<<endl;
			return false;
		}
	} // RMC
	else
	{
		// cerr<<"NMEA parser unknown message "<<Buffer<<endl;
		return false;
	}

	return false;

}

std::ostream& operator<<(std::ostream& s, const nmea_t& nmea)
{
	std::string ws(" ");
	s<<"utc:"<<nmea.utc<<ws;

	s<<"lat:"<<nmea.lat<<ws;
	s<<"lon:"<<nmea.lon<<ws;
	s<<"alt:"<<nmea.alt<<ws;

	s<<"speed_over_ground_mps:"<<nmea.speed_over_ground_mps<<ws;
	s<<"course_over_ground_deg:"<<nmea.course_over_ground_deg<<ws;

	s<<"sats:"<<nmea.sats<<ws;

	if(nmea.fix_status == nmea_t::fix_status_t::kValid)
		s<<"VALID"<<ws;
	else if(nmea.fix_status == nmea_t::fix_status_t::kInvalid)
		s<<"INVALID"<<ws;

	if( nmea.fix_quality == nmea_t::fix_quality_t::kNoFix )
		s<<"Q:kNoFix"<<ws;
	else if( nmea.fix_quality == nmea_t::fix_quality_t::kAutonomous )
		s<<"Q:kAutonomous"<<ws;
	else if( nmea.fix_quality == nmea_t::fix_quality_t::kDifferential )
		s<<"Q:kDifferential"<<ws;
	else if( nmea.fix_quality == nmea_t::fix_quality_t::kRtkFixed )
		s<<"Q:kRtkFixed"<<ws;
	else if( nmea.fix_quality == nmea_t::fix_quality_t::kRtkFloat )
		s<<"Q:kRtkFloat"<<ws;
	else if( nmea.fix_quality == nmea_t::fix_quality_t::kEstimated )
		s<<"Q:kEstimated"<<ws;

	return s;
}