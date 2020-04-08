#include <unistd.h> // write()
#include <signal.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "pigpio.h"

#include "mtx2/mtx2.h"
#include "nmea/nmea.h"
#include "ublox/ublox_cmds.h"
#include "ublox/ublox.h"
#include "ds18b20/ds18b20.h"


const unsigned int GPIOPIN_MTX2_EN_gpio = 22;
int radio_fd = 0;


char _hex(char Character)
{
	char _hexTable[] = "0123456789ABCDEF";
	return _hexTable[int(Character)];
}

std::string CRC(std::string i_str)
{
	using std::string;

	unsigned int CRC = 0xffff;
	// unsigned int xPolynomial = 0x1021;

	for (size_t i = 0; i < i_str.length(); i++)
	{
		CRC ^= (((unsigned int)i_str[i]) << 8);
		for (int j=0; j<8; j++)
		{
			if (CRC & 0x8000)
			    CRC = (CRC << 1) ^ 0x1021;
			else
			    CRC <<= 1;
		}
	}

	string result;
	result += _hex((CRC >> 12) & 15);
	result += _hex((CRC >> 8) & 15);
	result += _hex((CRC >> 4) & 15);
	result += _hex(CRC & 15);

	return result;
}


void CTRL_C(int sig)
{
	gpioWrite(GPIOPIN_MTX2_EN_gpio, 0);
	close(radio_fd);
	gpioTerminate();
	exit(0);
}


int main1()
{
    using namespace std;

	signal(SIGINT, CTRL_C);
	signal(SIGTERM, CTRL_C);

	system("sudo modprobe w1-gpio");

	if (gpioInitialise() < 0)
	{
		std::cerr<<"pigpio initialisation failed\n";
		return 1;
	}

    // RADIO
    //
	gpioSetPullUpDown(GPIOPIN_MTX2_EN_gpio, PI_PUD_DOWN);
	gpioSetMode(GPIOPIN_MTX2_EN_gpio, PI_OUTPUT);
	gpioWrite (GPIOPIN_MTX2_EN_gpio, 1);
	mtx2_set_frequency(GPIOPIN_MTX2_EN_gpio, 434.50f);
	radio_fd = mtx2_open("/dev/serial0", baud_t::k300);
    if (!radio_fd)
	{
		cerr<<"Failed opening radio UART /dev/serial0"<<endl;
		return 1;
	}
	cout<<"Radio FD "<<radio_fd<<endl;

    // uBLOX I2C
    //
    int uBlox_i2c_fd = uBLOX_i2c_open( "/dev/i2c-7", 0x42 );
	if (!uBlox_i2c_fd)
	{
		cerr<<"Failed opening I2C /dev/i2c-7 0x42"<<endl;
		return 1;
	}
    cout<<"uBLOX I2C FD "<<uBlox_i2c_fd<<endl;
	write(uBlox_i2c_fd, UBX_CMD_EnableOutput_ACK_ACK, sizeof(UBX_CMD_EnableOutput_ACK_ACK));
	write(uBlox_i2c_fd, UBX_CMD_EnableOutput_ACK_NAK, sizeof(UBX_CMD_EnableOutput_ACK_NAK));
	sleep(3);
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GSV_OFF, sizeof(UBX_CMD_GSV_OFF) );
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GLL_OFF, sizeof(UBX_CMD_GLL_OFF) );
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GSA_OFF, sizeof(UBX_CMD_GSA_OFF) );
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_VTG_OFF, sizeof(UBX_CMD_VTG_OFF) );
    while (!uBLOX_write_msg_ack(uBlox_i2c_fd, UBX_CMD_NAV5_Airbororne1G, sizeof(UBX_CMD_NAV5_Airbororne1G)))
        cout << "Retry Setting uBLOX Airborne1G Model" << endl;
	sleep(1);

    // DS18B20 temp sensor
    //
    const string ds18b20_device = find_ds18b20_device();
    if(ds18b20_device == "")
    {
		cerr << "Failed ds18b20_device "<<ds18b20_device<<endl;
        return 1;
    }
    cout<<"ds18b20_device "<<ds18b20_device<<endl;

    nmea_t nmea; // parsed GPS data
	int msg_num = 0;
	while(true)
	{
		msg_num++;

        // gps data
		//
		const vector<char> ublox_data = uBLOX_read_msg(uBlox_i2c_fd);
		const string nmea_str( NMEA_get_last_msg(ublox_data.data(), ublox_data.size()) );
		// cout<<nmea_str<<endl;
        if( !NMEA_msg_checksum_ok(nmea_str) )
        {
            cerr<<"NMEA Checksum Fail: "<<nmea_str<<endl;
            continue;
        }

		NMEA_parse( nmea_str.c_str(), nmea );
		const bool gps_fix_valid = nmea.fix_status == nmea_t::fix_status_t::kValid;

        // ds18b20
        //
        const float temperature_cels = read_temp_from_ds18b20(ds18b20_device);

        stringstream  msg_stream;
        // msg_stream<<nmea;
        msg_stream<<"not-a-real-flight";
        msg_stream<<","<<msg_num;
        msg_stream<<","<<nmea.utc;
        msg_stream<<","<<nmea.lat<<","<<nmea.lon<<","<<nmea.alt;
		msg_stream<<","<<nmea.sats<<","<<gps_fix_valid;
        // msg_stream<<","<<"05231.4567"<<","<<"2117.8412"<<","<<nmea.alt; // example NMEA format
        msg_stream<<","<<setprecision(1)<<fixed<<temperature_cels;

		const string msg_and_crc = string("\0",1) + "$$$" + msg_stream.str() + '*' + CRC(msg_stream.str());
        cout<<msg_and_crc<<endl;

		// emit RF
		//
		mtx2_write(radio_fd, msg_and_crc + '\n');
	}

    close(uBlox_i2c_fd);
	close(radio_fd);
	gpioWrite (GPIOPIN_MTX2_EN_gpio, 0);
	gpioTerminate();

    return 0;
}


int main(int argc, char** argv)
{
    return main1();
}
