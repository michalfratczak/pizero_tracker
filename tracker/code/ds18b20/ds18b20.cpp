#include "ds18b20.h"

#include <dirent.h>

#include <vector>
#include <string>
// #include <iostream>
#include <fstream>
#include <regex>


namespace
{

std::vector<std::string>  get_dir_content(const std::string dir)
{
    std::vector<std::string> ret;

    DIR* dp;
    if( (dp  = opendir(dir.c_str())) == NULL)
        return ret;

    struct dirent *dirp;
    while ((dirp = readdir(dp)) != NULL)
        ret.push_back(std::string(dirp->d_name));
    closedir(dp);

    return ret;
}

} // ns


std::string find_ds18b20_device(const std::string& base_dir) // "/sys/bus/w1/devices/"
{
    using namespace std;
    auto cont = get_dir_content( base_dir );
    for(const auto& f : cont)
        if( f.substr(0, 3) == "28-" )
            return base_dir + "/" + f + "/w1_slave";
    return "";
}


float read_temp_from_ds18b20(const std::string& device)
{
    using namespace std;
    ifstream ds18b20_file( device );
    if ( !ds18b20_file.is_open() )
        return -99;

    /*
    8a 01 4b 46 7f ff 06 10 2c : crc=2c YES
    8a 01 4b 46 7f ff 06 10 2c t=24625
    */

    vector<string> file_lines;
    string line;
    while ( getline (ds18b20_file,line) )
        file_lines.push_back(line);

    // crc
    //
    const std::regex rex_crc( R"_(^\s*?([\w\w\s]+): crc=(\w\w) (YES)\s*$)_" );
    smatch match_crc;
    bool crc_ok = false;
    for( const auto& l : file_lines )
    {
        regex_match(l, match_crc, rex_crc);
        if( match_crc.size() == 4 && match_crc[3] == "YES" )
        {
            crc_ok = true;
            break;
        }
    }

    if(!crc_ok)
        return -98;

    // temp
    //
    const std::regex rex_temp( R"_(^\s*?([\w\w\s]+) t=(\d+)\s*$)_" );
    smatch match_temp;
    for( const auto& l : file_lines )
    {
        regex_match(l, match_temp, rex_temp);
        if( match_temp.size() != 3)
            continue;
        float temp = stof( match_temp[2] ) / 1000.0f;
        return temp;
    }

    ds18b20_file.close();

    return -99;
}
