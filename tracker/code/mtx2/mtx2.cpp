#include "mtx2.h"

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>

#include <math.h>

#include <string>
#include <iostream>

#include "pigpio.h"


namespace
{


struct termios G_MTX2B_SERIAL_OPTS;

speed_t baud2speed(const baud_t baud)
{
	switch (baud)
	{
		case baud_t::k50: return B50;
		case baud_t::k75: return B75;
		case baud_t::k150: return B150;
		case baud_t::k200: return B200;
		case baud_t::k300: return B300;
		case baud_t::k600: return B600;
		case baud_t::k1200: return B1200;
		case baud_t::k4800: return B4800;
		case baud_t::k9600: return B9600;
	}
	std::cerr<<"Unknown baud_t value. Default to 50 BAUD "<<std::endl;
	return B50;
}

} // ns

int mtx2_open(const std::string i_device, const baud_t baud) //"/dev/serial0"
{

	if(baud == baud_t::kInvalid)
	{
		std::cerr<<"Invalid baud."<<std::endl;
		return 0;
	}

	int fd = open(i_device.c_str(), O_WRONLY | O_NOCTTY);	// O_NDELAY);
	if(!fd)
	{
		std::cerr<<"Error opening "<<i_device<<std::endl;
		return 0;
	}


	// get the current options
	tcgetattr(fd, &G_MTX2B_SERIAL_OPTS);

	// set raw input
	G_MTX2B_SERIAL_OPTS.c_lflag &= ~ECHO;
	G_MTX2B_SERIAL_OPTS.c_cc[VMIN]  = 0;
	G_MTX2B_SERIAL_OPTS.c_cc[VTIME] = 10;

	speed_t speed = baud2speed(baud);
	cfsetispeed(&G_MTX2B_SERIAL_OPTS, speed);
	cfsetospeed(&G_MTX2B_SERIAL_OPTS, speed);

	G_MTX2B_SERIAL_OPTS.c_cflag |= CSTOPB;
	// G_MTX2B_SERIAL_OPTS.c_cflag &= ~CSTOPB;
	G_MTX2B_SERIAL_OPTS.c_cflag |= CS8;
	G_MTX2B_SERIAL_OPTS.c_oflag &= ~ONLCR;
	G_MTX2B_SERIAL_OPTS.c_oflag &= ~OPOST;
	G_MTX2B_SERIAL_OPTS.c_iflag &= ~IXON;
	G_MTX2B_SERIAL_OPTS.c_iflag &= ~IXOFF;

	tcsetattr(fd, TCSANOW, &G_MTX2B_SERIAL_OPTS);

	return fd;
}


int mtx2_write(const int serial_file_descriptor, const char* msg, const size_t msg_sz)
{
	int bytes_written = write( serial_file_descriptor, msg, msg_sz );
	tcsetattr( serial_file_descriptor, TCSAFLUSH, &G_MTX2B_SERIAL_OPTS );
	return bytes_written;
}


int mtx2_write(const int serial_file_descriptor, const std::string& msg)
{
	return mtx2_write( serial_file_descriptor, msg.c_str(), msg.size() );
}


void mtx2_set_frequency(const int mtx2_enable_pin_gpio, const float freq_Mhz) // 434.250
{
	/*
	Frequency (in MHz) = 6.5 x (integer + (fraction / 2^19))
	Example:
		430MHz divide by 6.5 equals 66.15384615 so
			"integer" = 65
			fraction = (0.15384615 +1) x 524288  =  604948
		and to confirm: 6.5 x (65+(604948/524288)) does indeed equal 430MHz (plus 3.9Hz)
	*/

	using namespace std;

	const float _mtx2comp = (freq_Mhz + 0.0015f) / 6.5f;
	const unsigned int _mtx2int = floor(_mtx2comp) - 1;
	const long _mtx2fractional = (_mtx2comp-_mtx2int) * 524288;

	// cout<<"_mtx2comp "<<_mtx2comp<<endl;
	// cout<<"_mtx2int "<<_mtx2int<<endl;
	// cout<<"_mtx2fractional "<<fixed<<_mtx2fractional<<endl;
	// cout<<"Confirm "<<( 6.5 * ((float)_mtx2int + ((float)_mtx2fractional/524288)) )<<endl;

	char _mtx2command[17];
	snprintf( _mtx2command, 17, "@PRG_%02X%06lX\r", _mtx2int, _mtx2fractional );
	// for(size_t i=0; i<sizeof(_mtx2command); ++i)		_mtx2command[i] = ~ _mtx2command[i];
	// printf("MTX2 command  is %s\n", _mtx2command);

	gpioWaveAddNew();
	gpioWaveAddSerial(mtx2_enable_pin_gpio, 9600, 8, 2, 0, sizeof(_mtx2command), _mtx2command);
	const int wave_id = gpioWaveCreate();

	int repetitions = 2;
	while ( repetitions-- && wave_id >= 0 )
	{
		gpioWaveTxSend(wave_id, 0);
		while (gpioWaveTxBusy())
			time_sleep(0.1);
	}

}


int baud_t_to_int(const baud_t& baud)
{
	switch (baud)
	{
		case baud_t::k50: return 50;
		case baud_t::k75: return 75;
		case baud_t::k150: return 150;
		case baud_t::k200: return 200;
		case baud_t::k300: return 300;
		case baud_t::k600: return 600;
		case baud_t::k1200: return 1200;
		case baud_t::k4800: return 4800;
		case baud_t::k9600: return 9600;
	}
	return 0;
}


baud_t baud_t_from_int(const int baud)
{
	switch (baud)
	{
		case 50: return		baud_t::k50;
		case 75: return		baud_t::k75;
		case 150: return	baud_t::k150;
		case 200: return	baud_t::k200;
		case 300: return	baud_t::k300;
		case 600: return	baud_t::k600;
		case 1200: return	baud_t::k1200;
		case 4800: return	baud_t::k4800;
		case 9600: return	baud_t::k9600;
	}
	return baud_t::kInvalid;
}
