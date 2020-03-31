#include <unistd.h> // write()
#include <signal.h>

#include <string>
#include <iostream>

#include "wiringPi.h"

#include "mtx2b/mtx2b.h"

const int GPIOPIN_MTX2B_EN = 3;
int radio_fd = 0;


void CTRL_C(int sig)
{
	close(radio_fd);
	digitalWrite (GPIOPIN_MTX2B_EN, 0);
	exit(0);
}


int main()
{
	signal(SIGINT, CTRL_C);

	wiringPiSetup();
	system("sudo modprobe w1-gpio");

	pinMode (GPIOPIN_MTX2B_EN, OUTPUT);
	digitalWrite (GPIOPIN_MTX2B_EN, 1);

	radio_fd = mtx2b_open("/dev/serial0", baud_t::k300); //"/dev/serial0"
	std::cout<<"radio_fd "<<radio_fd<<std::endl;

	int count = 50;
	while(count--)
	{
		std::string msg("message on radio waves !");
		std::cout<<msg<<" ";
		std::cout<<mtx2b_write(radio_fd, msg)<<std::endl;
	}

	digitalWrite (GPIOPIN_MTX2B_EN, 0);
}