
#include "dynamics_t.h"

#include <sstream>
#include <iomanip>

// utc_seconds is HHMMSS converted to seconds. return true if sample was accepted
bool dynamics_t::add(const tp timestamp, const float val)
{
    if( !initialised_ && timestamp.time_since_epoch().count() )
    {
        val_ = val;
        val_prev_ = val;
        val_prev_timestamp_ = timestamp;
        dVdT_ = 0;

        if(val > val_max_)
            val_max_ = val;
        if(val < val_min_)
            val_min_ = val;

        initialised_ = true;
        return true;
    }

    if( (timestamp - val_prev_timestamp_).count() < min_dT_ )
        return false;

    dVdT_ = (val - val_prev_) / float((timestamp - val_prev_timestamp_).count());

    if(val > val_max_)
        val_max_ = val;

    if(val < val_min_)
        val_min_ = val;

    val_prev_ = val;
    val_prev_timestamp_ = timestamp;

    return true;
}

std::string dynamics_t::json() const
{
    std::stringstream ss;
    std::string sep(",");
    ss<<"{";
        ss<<"'initialised':"<<initialised_<<sep;
        ss<<"'val':"<<val_<<sep;
        ss<<"'min':"<<val_min_<<sep;
        ss<<"'max':"<<val_max_<<sep;
        ss<<"'dVdT':"<<dVdT_;
    ss<<"}";
    return ss.str();
}
