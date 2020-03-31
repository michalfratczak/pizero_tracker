#include "mtx2b.h"

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
// #include <errno.h>   	// Error number definitions
// #include <sys/types.h>
// #include <sys/stat.h>

#include <string>
#include <iostream>

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
	}
	return B50;
}

} // ns

int mtx2b_open(const std::string i_device, const baud_t baud) //"/dev/serial0"
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

	speed_t speed = baud2speed(baud);

	// get the current options
	tcgetattr(fd, &G_MTX2B_SERIAL_OPTS);

	// set raw input
	G_MTX2B_SERIAL_OPTS.c_lflag &= ~ECHO;
	G_MTX2B_SERIAL_OPTS.c_cc[VMIN]  = 0;
	G_MTX2B_SERIAL_OPTS.c_cc[VTIME] = 10;

	G_MTX2B_SERIAL_OPTS.c_cflag |= CSTOPB;
	cfsetispeed(&G_MTX2B_SERIAL_OPTS, speed);
	cfsetospeed(&G_MTX2B_SERIAL_OPTS, speed);
	G_MTX2B_SERIAL_OPTS.c_cflag |= CS8;
	G_MTX2B_SERIAL_OPTS.c_oflag &= ~ONLCR;
	G_MTX2B_SERIAL_OPTS.c_oflag &= ~OPOST;
	G_MTX2B_SERIAL_OPTS.c_iflag &= ~IXON;
	G_MTX2B_SERIAL_OPTS.c_iflag &= ~IXOFF;

	tcsetattr(fd, TCSANOW, &G_MTX2B_SERIAL_OPTS);

	return fd;
}

int mtx2b_write(const int fd, const std::string& msg)
{
	int bytes_written = write( fd, msg.c_str(), msg.size() );
	tcsetattr( fd, TCSAFLUSH, &G_MTX2B_SERIAL_OPTS );
	return bytes_written;
}