#include "GLOB.h"

#include <sstream>

std::string GLOB::str() const
{
    std::stringstream s;

    s<<"\tcallsign="<<cli.callsign<<"\n";
    s<<"\tfreqMHz="<<cli.freqMHz<<"\n";
    s<<"\tbaud="<<static_cast<int>(cli.baud)<<"\n";
    s<<"\tssdv_image="<<cli.ssdv_image<<"\n";
    s<<"\tmsg_num="<<cli.msg_num<<"\n";
    s<<"\tport="<<cli.port<<"\n";
    s<<"\thw_pin_radio_on="<<cli.hw_pin_radio_on<<"\n";
    s<<"\thw_radio_serial="<<cli.hw_radio_serial<<"\n";
    s<<"\thw_ublox_device="<<cli.hw_ublox_device<<"\n";

    return s.str();
}
