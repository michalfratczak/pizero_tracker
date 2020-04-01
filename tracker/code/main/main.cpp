#include <unistd.h> // write()
#include <signal.h>

#include <string>
#include <iostream>
#include <sstream>

#include "pigpio.h"

#include "mtx2/mtx2.h"
#include "nmea/nmea.h"
#include "ublox/ublox_cmds.h"
#include "ublox/ublox.h"
#include "ds18b20/ds18b20.h"


const unsigned int GPIOPIN_MTX2_EN_gpio = 22;
int radio_fd = 0;


void CTRL_C(int sig)
{
	close(radio_fd);
	gpioWrite(GPIOPIN_MTX2_EN_gpio, 0);
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
	const std::string radio_serial_device("/dev/serial0");
	radio_fd = mtx2_open(radio_serial_device, baud_t::k50);
    cout<<"Radio FD "<<radio_fd<<endl;

    // uBLOX I2C
    //
    int uBlox_i2c_fd = uBLOX_i2c_open( "/dev/i2c-7", 0x42 );
	if (!uBlox_i2c_fd)
	{
		cerr << "Failed opening I2C" << endl;
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
    cout<<"ds18b20_device "<<ds18b20_device<<endl;
    if(ds18b20_device == "")
    {
		cerr << "Failed ds18b20_device "<<ds18b20_device<<endl;
        return 1;
    }

    nmea_t nmea; // parsed GPS data
	while(true)
	{
        // gps data
		const vector<char> nmea_data = uBLOX_read_msg(uBlox_i2c_fd);
		const string nmea_str(vec2str(nmea_data));
        if( !NMEA_msg_checksum_ok(nmea_str) )
        {
            cerr<<"NMEA Checksum Fail: "<<nmea_str<<endl;
            continue;
        }
		NMEA_parse( nmea_str.c_str(), nmea );

        // ds18b20
        //
        const float temperature_cels = read_temp_from_ds18b20(ds18b20_device);

        // radio message
        stringstream  radio_msg_stream;
        radio_msg_stream<<"$ ";
        radio_msg_stream<<nmea;
        radio_msg_stream<<" temp:"<<temperature_cels;

        const string radio_msg_string = radio_msg_stream.str();
        cout<<radio_msg_string<<endl;

		mtx2_write(radio_fd, radio_msg_string);
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
