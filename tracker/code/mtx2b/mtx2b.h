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
	k1200 = 1200
};

int mtx2b_open(const std::string i_device, const baud_t baud); //"/dev/serial0"
int mtx2b_write(const int fd, const std::string& msg);
