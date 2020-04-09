#include "ssdv_t.h"

#include <iostream>

size_t ssdv_t::load_file(const std::string file_path)
{
    FILE* p_file = fopen(file_path.c_str(), "rb");
    if(!p_file)
        return 0;

    tile_t tile;
    size_t total_tiles = 0;
    while(1)
    {
        const size_t read_bytes = fread( tile.data(), sizeof(char), 256, p_file );
        if(!read_bytes)
            return  total_tiles;
        tiles_que_.push_back(tile);
        ++total_tiles;
    }
}

ssdv_t::tile_t ssdv_t::next_tile()
{
    tile_t tile = tiles_que_.front();
    tiles_que_.pop_front();
    return tile;
}
