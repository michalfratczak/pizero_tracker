#include <unistd.h> // write()
#include <signal.h>

#include <string>
#include <iostream>

#include "wiringPi.h"

#include "mtx2/mtx2.h"

const int GPIOPIN_MTX2_EN = 3;
int radio_fd = 0;


void CTRL_C(int sig)
{
	close(radio_fd);
	digitalWrite (GPIOPIN_MTX2_EN, 0);
	exit(0);
}


int main()
{
	signal(SIGINT, CTRL_C);

	wiringPiSetup();
	system("sudo modprobe w1-gpio");

	pinMode (GPIOPIN_MTX2_EN, INPUT);
	pullUpDnControl(GPIOPIN_MTX2_EN, PUD_DOWN);
	pinMode (GPIOPIN_MTX2_EN, OUTPUT);

	const std::string radio_serial_device("/dev/serial0");
	mtx2_set_frequency(radio_serial_device, 434.580f);
	mtx2_set_frequency(radio_serial_device, 434.580f);

	radio_fd = mtx2_open(radio_serial_device, baud_t::k50); //"/dev/serial0"
	digitalWrite (GPIOPIN_MTX2_EN, 1);

	int count = 10;
	while(count--)
	{
		std::string msg("message on radio waves !");
		std::cout<<msg<<" "<<count<<std::endl;
		std::cout<<"bytes: "<<mtx2_write(radio_fd, msg)<<std::endl;
		std::cout<<"-----"<<std::endl;
		sleep(2);
	}

	close(radio_fd);
	digitalWrite (GPIOPIN_MTX2_EN, 0);
}
