#include "nmea.h"

// #include <stdio.h>
#include <math.h>
#include <cstring>
#include <iostream>
#include <sstream>


namespace
{

float degree_2_decimal(float i_pos)
{
	float degs = trunc(i_pos / 100);
	float mins = i_pos - 100.0f * degs;
	float res = degs + mins / 60.0f;
	return res;
}

} // ns

int NMEA_checksum(const char* buff, const size_t buff_sz)
{
	int c = 0;
	for (size_t i = 0; i < buff_sz; ++i)
		c ^= *buff++;
	return c;
}


bool NMEA_msg_checksum_ok(const char* msg, const size_t msg_sz)
{
	int chck = NMEA_checksum(msg+1, msg_sz-4); // without $, up to *
	char chck_str[2];
	sprintf(chck_str, "%02X", chck);

	bool checksum_is_equal =
		chck_str[0] == *(msg+msg_sz-2) && chck_str[1] == *(msg+msg_sz-1);

	return checksum_is_equal;
}


bool NMEA_msg_checksum_ok(const std::string& msg)
{
	return NMEA_msg_checksum_ok( msg.c_str(), msg.size() );
}


// $GNGGA,170814.00,,,,,0,00,99.99,,,,,,*73
// return false if not found
// output:
//		buff+in points to $
// 		buff+out points to second char of checksum (3)
bool NMEA_get_last_msg(const char* buff, const size_t buff_sz, size_t& in, size_t& out)
{
	// find end of message: *
	const char* buff_star = buff + buff_sz - 1;
	while ( buff <= buff_star && *buff_star != '*' )
		buff_star--;

	if(buff <= buff_star)
		out = buff_star - buff + 2;
	else
		return false;

	const char* buff_dollar = buff_star;
	while ( buff <= buff_dollar && *buff_dollar != '$' )
		buff_dollar--;

	if(buff <= buff_dollar)
		in = buff_dollar - buff;
	else
		return false;

	return true;
}


std::string NMEA_get_last_msg(const char* buff, const size_t buff_sz)
{
	size_t in;
	size_t out;
	bool has_msg = NMEA_get_last_msg(buff, buff_sz, in, out);
	if(has_msg)
		return std::string(buff+in, buff+out+1);
	else
		return "";
}


std::string NMEA_get_last_msg(const std::string& msg)
{
	return  NMEA_get_last_msg( msg.c_str(), msg.size() );
}


bool NMEA_parse(const char* Buffer, nmea_t& o_nmea)
{
	using namespace std;

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
	char speedstring[16];
	char coursestring[16];

	if (strncmp(Buffer + 3, "GGA", 3) == 0) // Global positioning system fix data
	{
		int scanned_positions =
			sscanf(Buffer + 7, "%f,%f,%c,%f,%c,%d,%d,%f,%f,%c", &
				utc, &lat, &ns, &lon, &ew, &quality, &sats, &hdop, &alt, &alt_units);

		if(scanned_positions >= 10)
		{
			lat = degree_2_decimal(lat);
			if(ns == 'S')
				lat = -lat;

			lon = degree_2_decimal(lon);
			if(ew == 'W')
				lon = -lon;

			sprintf( o_nmea.utc, "%06d", int(floor(utc)) );
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
		else if(scanned_positions > 0 && utc != 0)
		{
			sprintf( o_nmea.utc, "%06d", int(floor(utc)) );
			return true;
		}
		else
		{
			// cerr<<"GGA scanf err"<<endl; // probably not all fields in sentence
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

		if(scanned_positions >= 7)
		{
			sprintf( o_nmea.utc, "%06d", int(floor(utc)) );

			lat = degree_2_decimal(lat);
			if(ns == 'S')
				lat = -lat;
			o_nmea.lat = lat;

			lon = degree_2_decimal(lon);
			if(ew == 'W')
				lon = -lon;
			o_nmea.lon = lon;

			// o_nmea.alt = alt; // there is no alt in RMC
			// o_nmea.sats = sats; // the is no sats in RMC
			o_nmea.speed_over_ground_mps = atof(speedstring);
			o_nmea.course_over_ground_deg = atof(coursestring);

			if(status == 'A')
				o_nmea.fix_status = nmea_t::fix_status_t::kValid;
			else if(status == 'V')
				o_nmea.fix_status = nmea_t::fix_status_t::kInvalid;

			return true;
		}
		else if(scanned_positions > 0 && utc != 0)
		{
			sprintf( o_nmea.utc, "%06d", int(floor(utc)) );
			return true;
		}
		else
		{
			// cerr<<"RMC scanf err"<<endl; // probably not all fields in sentence
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


std::string nmea_t::str() const
{
	std::stringstream s;
	std::string ws(",");

	s<<"utc:"<<utc<<ws;

	s<<"lat:"<<lat<<ws;
	s<<"lon:"<<lon<<ws;
	s<<"alt:"<<alt<<ws;

	s<<"speed_over_ground_mps:"<<speed_over_ground_mps<<ws;
	s<<"course_over_ground_deg:"<<course_over_ground_deg<<ws;

	s<<"sats:"<<sats<<ws;

	if(fix_status == nmea_t::fix_status_t::kValid)
		s<<"VALID"<<ws;
	else if(fix_status == nmea_t::fix_status_t::kInvalid)
		s<<"INVALID"<<ws;

	if( fix_quality == nmea_t::fix_quality_t::kNoFix )
		s<<"Q:kNoFix";
	else if( fix_quality == nmea_t::fix_quality_t::kAutonomous )
		s<<"Q:kAutonomous";
	else if( fix_quality == nmea_t::fix_quality_t::kDifferential )
		s<<"Q:kDifferential";
	else if( fix_quality == nmea_t::fix_quality_t::kRtkFixed )
		s<<"Q:kRtkFixed";
	else if( fix_quality == nmea_t::fix_quality_t::kRtkFloat )
		s<<"Q:kRtkFloat";
	else if( fix_quality == nmea_t::fix_quality_t::kEstimated )
		s<<"Q:kEstimated";

	return s.str();
}



std::string nmea_t::json() const
{
	std::stringstream s;
	std::string sep(",");

	s<<"{";

	s<<"'utc':"<<utc<<sep;
	s<<"'lat':"<<lat<<sep;
	s<<"'lon':"<<lon<<sep;
	s<<"'alt':"<<alt<<sep;

	s<<"'speed_over_ground_mps':"<<speed_over_ground_mps<<sep;
	s<<"'course_over_ground_deg':"<<course_over_ground_deg<<sep;

	s<<"'sats':"<<sats<<sep;

	if(fix_status == nmea_t::fix_status_t::kValid)
		s<<"'fixStatus':'VALID'"<<sep;
	else if(fix_status == nmea_t::fix_status_t::kInvalid)
		s<<"'fixStatus':'INVALID'"<<sep;

	if( fix_quality == nmea_t::fix_quality_t::kNoFix )
		s<<"'Q':'kNoFix'";
	else if( fix_quality == nmea_t::fix_quality_t::kAutonomous )
		s<<"'Q:kAutonomous'";
	else if( fix_quality == nmea_t::fix_quality_t::kDifferential )
		s<<"'Q:kDifferential'";
	else if( fix_quality == nmea_t::fix_quality_t::kRtkFixed )
		s<<"'Q:kRtkFixed'";
	else if( fix_quality == nmea_t::fix_quality_t::kRtkFloat )
		s<<"'Q:kRtkFloat'";
	else if( fix_quality == nmea_t::fix_quality_t::kEstimated )
		s<<"'Q:kEstimated'";

	s<<"}";

	return s.str();
}


// convert utc HHMMSS to seconds
int nmea_t::utc_as_seconds() const
{
	int H=0;
	int M=0;
	int S=0;
	sscanf(utc, "%02d%02d%02d", &H, &M, &S);
	const int total_seconds = S + 60*M + 60*60*H;
	return total_seconds;
}
