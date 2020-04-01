#include <string>

std::string find_ds18b20_device(const std::string& base_dir = "/sys/bus/w1/devices/" );

float read_temp_from_ds18b20(const std::string& device);
