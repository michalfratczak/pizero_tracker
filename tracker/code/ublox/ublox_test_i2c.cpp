
#include <string>
#include <iostream>

#include "ublox/ublox_cmds.h"
#include "ublox/ublox.h"
#include "nmea/nmea.h"


void ublox_test_i2c(void)
{
	using namespace std;

	int uBlox_i2c_fd = uBLOX_i2c_open( "/dev/i2c-7", 0x42 );
	if (!uBlox_i2c_fd)
	{
		cerr << "Failed opening I2C" << endl;
		return;
	}

	// config
	//
	int written_bytes = 0;
	written_bytes = write(uBlox_i2c_fd, UBX_CMD_EnableOutput_ACK_ACK, sizeof(UBX_CMD_EnableOutput_ACK_ACK));
	written_bytes = write(uBlox_i2c_fd, UBX_CMD_EnableOutput_ACK_NAK, sizeof(UBX_CMD_EnableOutput_ACK_NAK));
	sleep(3);

	// Disable GSV - GNSS Satellites in View
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GSV_OFF, sizeof(UBX_CMD_GSV_OFF) );

	// Disable GLL - Latitude and longitude, with time of position fix and status
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GLL_OFF, sizeof(UBX_CMD_GLL_OFF) );

	// Disable GSA - GNSS DOP and Active Satellites
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GSA_OFF, sizeof(UBX_CMD_GSA_OFF) );

	// Disable VTG - Course over ground and Ground speed
	uBLOX_write_msg_ack(uBlox_i2c_fd, UBX_CMD_VTG_OFF, sizeof(UBX_CMD_VTG_OFF));

	nmea_t nmea;

	long int msg_cnt = 0;
	while (1)
	{
		vector<char> msg_data = uBLOX_read_msg(uBlox_i2c_fd);
		string msg(vec2str(msg_data));

		cout<<msg_cnt<<": "<<msg<<" "<<NMEA_msg_checksum_ok(msg)<<endl;
		NMEA_parse( msg.c_str(), nmea );
		cout<<nmea<<endl;

		msg_cnt++;
		if (msg_cnt == 15)
		{
			cout << "Setting uBLOX Pedestrian Model" << endl;
			// while (!uBLOX_write_msg_ack(uBlox_i2c_fd, UBX_CMD_NAV5_Airbororne1G, sizeof(UBX_CMD_NAV5_Airbororne1G)))
			// 	cout << "Retry Setting uBLOX AirBorne Model" << endl;
			while (!uBLOX_write_msg_ack(uBlox_i2c_fd, UBX_CMD_NAV5_Pedestrian, sizeof(UBX_CMD_NAV5_Pedestrian)))
				cout << "Retry Setting uBLOX Pedestrian Model" << endl;
		}
	}
}

int main(int argc, char **argv)
{
	ublox_test_i2c();
	return 0;
}
