#pragma once

#include <string>

int mtx2b_open(const std::string i_device, const int baud); //"/dev/serial0"
int mtx2b_write(const int fd, const std::string& msg);
