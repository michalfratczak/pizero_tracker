
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

std::chrono::system_clock::time_point dynamics_t::utc2tp(const std::string utc)
{
	using namespace std;

	// init with *now*
	time_t _now = time(nullptr);   // "now: as long int
	tm tm = *localtime(&_now); // init all fields

	istringstream ss(utc);
	ss >> get_time( &tm, "%H%M%S" ); // get HHMMSS

	return	chrono::system_clock::from_time_t( mktime(&tm) );
}