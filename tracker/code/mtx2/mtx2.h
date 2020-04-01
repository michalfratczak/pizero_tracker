#pragma once

#include <string>

enum class baud_t {
	kInvalid = 0,
	k50 = 50,
	k75 = 75,
	k100 = 100,
	k150 = 150,
	k200 = 200,
	k300 = 300,
	k600 = 600,
	k1200 = 1200,
	k4800 = 4800,
	k9600 = 9600
};

// returns file descriptor
int mtx2_open(const std::string i_device, const baud_t baud); //"/dev/serial0"

int mtx2_write(const int serial_file_descriptor, const char* msg, const size_t msg_sz);
int mtx2_write(const int serial_file_descriptor, const std::string& msg);

// will open and close i_device
// void mtx2_set_frequency(const std::string i_device, const float freq_Mhz); // 434.250
void mtx2_set_frequency(const int mtx2_enable_pin, const float freq_Mhz); // 434.250
