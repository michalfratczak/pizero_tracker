#pragma once

#include <string>
#include <tuple>
#include <map>
#include <mutex>
#include <chrono>

#include "GLOB.h"

// each process can periodically ping this class:
// while(1) {
//      ...
//      pulse.ping("uBLOX");
// }
//
// it is then possible to check if all processes are alive
// with get_oldest_ping_age()

class pulse_t
{
private:
	using map_t = std::map< std::string, std::chrono::high_resolution_clock::time_point >;
	map_t      map_;
	std::mutex  mtx_;

public:
	void ping(const std::string& name)
	{
		std::lock_guard<std::mutex> _lock(mtx_);
		map_[name] = std::chrono::high_resolution_clock::now();
	}

	std::tuple<long long, std::string> get_oldest_ping_age() // return oldest ping age in microseconds
	{
		using namespace std::chrono;
		auto now = high_resolution_clock::now();
		long long oldest_age = 0;
		std::string oldest_proc;

		{
			std::lock_guard<std::mutex> _lock(mtx_);
			for(const auto& proc : map_) {
				const auto age = duration_cast<microseconds>(now-proc.second).count();
				if(age > oldest_age) {
					oldest_age = age;
					oldest_proc = proc.first;
				}
			}
		}

		return std::make_tuple(oldest_age, oldest_proc);
	}

};

