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

speed_t baud2speed(const int baud)
{
	switch (baud)
	{
		case 50: return B50;
		case 75: return B75;
		case 150: return B150;
		case 200: return B200;
		case 300: return B300;
		case 600: return B600;
		case 1200: return B1200;
	}

	return B50;
}

} // ns

int mtx2b_open(const std::string i_device, const int baud) //"/dev/serial0"
{

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