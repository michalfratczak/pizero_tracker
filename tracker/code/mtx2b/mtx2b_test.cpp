#include <string>
#include <iostream>

#include "mtx2b/mtx2b.h"

int main()
{
	int radio_fd = mtx2b_open("/dev/serial0", 300); //"/dev/serial0"
	std::cout<<"radio_fd "<<radio_fd<<std::endl;

	std::string msg("message on radio waves !");
	std::cout<<mtx2b_write(radio_fd, msg)<<std::endl;
}