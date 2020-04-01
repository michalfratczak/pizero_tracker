#include <unistd.h> // write()
#include <signal.h>

#include <string>
#include <iostream>

#include "pigpio.h"

#include "mtx2/mtx2.h"

// const int GPIOPIN_MTX2_EN_wPI = 3;
const unsigned int GPIOPIN_MTX2_EN_gpio = 22;
int radio_fd = 0;


void CTRL_C(int sig)
{
	close(radio_fd);
	gpioWrite(GPIOPIN_MTX2_EN_gpio, 0);
	gpioTerminate();
	exit(0);
}


int main()
{
	signal(SIGINT, CTRL_C);
	signal(SIGKILL, CTRL_C);
	signal(SIGTERM, CTRL_C);

	system("sudo modprobe w1-gpio");

	if (gpioInitialise() < 0)
	{
		std::cerr<<"pigpio initialisation failed\n";
		return 1;
	}


	gpioSetPullUpDown(GPIOPIN_MTX2_EN_gpio, PI_PUD_DOWN);
	gpioSetMode(GPIOPIN_MTX2_EN_gpio, PI_OUTPUT);
	gpioWrite (GPIOPIN_MTX2_EN_gpio, 1);

	mtx2_set_frequency(GPIOPIN_MTX2_EN_gpio, 434.50f);

	const std::string radio_serial_device("/dev/serial0");
	radio_fd = mtx2_open(radio_serial_device, baud_t::k50); //"/dev/serial0"


	int count = 30;
	while(count--)
	{
		std::string msg("message on radio waves !");
		std::cout<<msg<<" "<<count<<std::endl;

		gpioWrite (GPIOPIN_MTX2_EN_gpio, 1);
		std::cout<<"bytes: "<<mtx2_write(radio_fd, msg)<<std::endl;
		gpioWrite (GPIOPIN_MTX2_EN_gpio, 0);
		std::cout<<"-----"<<std::endl;

		sleep(2);
	}

	close(radio_fd);
	gpioWrite (GPIOPIN_MTX2_EN_gpio, 0);
	gpioTerminate();
}
