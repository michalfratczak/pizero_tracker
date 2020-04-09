#include "GLOB.h"

#include <sstream>

std::string GLOB::str() const
{
    std::stringstream s;

    s<<"callsign="<<cli.callsign<<"\n";
    s<<"freqMHz="<<cli.freqMHz<<"\n";
    s<<"baud="<<baud_t_to_int(cli.baud)<<"\n";
    s<<"ssdv_image="<<cli.ssdv_image<<"\n";
    s<<"hw_pin_radio_on="<<cli.hw_pin_radio_on<<"\n";
    s<<"hw_radio_serial="<<cli.hw_radio_serial<<"\n";
    s<<"hw_ublox_device="<<cli.hw_ublox_device<<"\n";

    return s.str();
}
