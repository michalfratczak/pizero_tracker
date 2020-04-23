#pragma once

#include <string>
#include <array>
#include <deque>

// load SSDV encoded files and keep as stack of 256 bytes tiles
class ssdv_t
{

public:
    using packet_t = std::array<char, 256>;
    size_t size() const   { return tiles_que_.size(); }
    size_t load_file(const std::string file_path);
    packet_t next_packet(); // pop packet from que and return

private:
    std::deque<packet_t>  tiles_que_;

};
