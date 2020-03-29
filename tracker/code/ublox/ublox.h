#pragma once

#include <cinttypes>
// #include <cstddef>
#include <vector>
#include <string>

#include <unistd.h> // write()

int uBLOX_i2c_open(const std::string i_device, const unsigned char addr); // "/dev/i2c-7", 0x42

// read char from file descriptor
// and returns a std:string message (anything  between 255 or /n characters);
// sometimes returns just \n
// usec_sleep = sleep  microseconds when uBLOX talks 255
std::vector<char> uBLOX_read_msg(int fd, int usec_sleep = 3e5);

// write message with ACK
// wait - how many return lines to check for ACK
bool uBLOX_write_msg_ack(int fd, uint8_t* p_msg, size_t msg_sz, const size_t wait = 10);

std::string vec2str(std::vector<char> v);

