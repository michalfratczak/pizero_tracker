#pragma once

#include <string>
#include <array>
#include <deque>

// load SSDV encoded files and keep as stack of 256 bytes tiles
class ssdv_t
{

public:
    using tile_t = std::array<char, 256>;
    size_t size() const   { return tiles_que_.size(); }
    size_t load_file(const std::string file_path);
    tile_t next_tile(); // pop tile from que and return

private:
    std::deque<tile_t>  tiles_que_;

};
