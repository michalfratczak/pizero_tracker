
#include "dynamics_t.h"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>


void dynamics_t::add(const tp timestamp, const float val)
{
    val_ = val;
    val_min_ = std::min(val_, val_min_);
    val_max_ = std::max(val_, val_max_);

    if( !initialised_ && timestamp.time_since_epoch().count() )
    {
        val_prev_ = val;
        val_prev_timestamp_ = timestamp;
        dVdT_ = 0;
        initialised_ = true;
        return;
    }

    const float dT = float((timestamp - val_prev_timestamp_).count())/1e9;
    if( dT < min_dT_ )
        return;

    const float dV = (val_ - val_prev_);
    dVdT_ = dV / dT;
    dVdT_avg_.add(dVdT_);
    val_prev_ = val;
    val_prev_timestamp_ = timestamp;

    /*std::cout   <<"\n\tval "<<val
                <<"\n\tdV "<<dV
                <<"\n\tdT "<<dT
                <<"\n\tdV/dT "<<dVdT_<<std::endl;*/
}


std::string dynamics_t::str() const
{
    std::stringstream ss;
    std::string sep(", ");
    ss<<"'initialised':"<<initialised_<<sep;
    ss<<"'val':"<<val_<<sep;
    ss<<"'min':"<<val_min_<<sep;
    ss<<"'max':"<<val_max_<<sep;
    ss<<"'dVdT':"<<dVdT_<<sep;
    ss<<"'dVdT_avg':"<<dVdT_avg_.get();
    return ss.str();
}


std::string dynamics_t::json() const
{
    return std::string( "{" + str() + "}" );
}
