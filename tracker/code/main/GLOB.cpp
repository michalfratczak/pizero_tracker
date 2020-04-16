#include "GLOB.h"

#include <sstream>
#include <iostream>

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


void GLOB::dynamics_add(const std::string& name, const dynamics_t::tp timestamp, const float value)
{
    auto d = dynamics_.find(name);
    if( d == dynamics_.end() )
    {
        dynamics_[name] = dynamics_t();
        dynamics_[name].add(timestamp, value);
    }
    else
    {
        d->second.add(timestamp, value);
    }

}

dynamics_t GLOB::dynamics_get(const std::string& name) const
{
    auto d = dynamics_.find(name);
    if( d == dynamics_.end() )
        return dynamics_t(); // default, uninitialised
    else
        return d->second;
}

std::vector<std::string> GLOB::dynamics_keys() const
{
    std::vector<std::string> ret;
    for(const auto& k : dynamics_)
        ret.push_back(k.first);
    return ret;
}
