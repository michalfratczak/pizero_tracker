#pragma once

#include <string>
#include <chrono>
#include <limits>

// record min, max, dV/dT for a value
class dynamics_t
{
public:
    using tp = std::chrono::steady_clock::time_point;

private:
    bool    initialised_ = false;
    float   val_ = 0;
    float   val_prev_ = 0;
    tp      val_prev_timestamp_; // timestamp of previous value
    float   val_min_ = std::numeric_limits<float>::max();
    float   val_max_ = -std::numeric_limits<float>::min();
    float   dVdT_ = 0;

public:
    int     min_dT_ = 10; // minimum time difference to compute dV/dT. Sample too close to previous will be rejected

    bool    initialised() const { return initialised_; }

    bool    add(const tp timestamp, const float val); // utc_seconds is HHMMSS converted to seconds. return true if sample was accepted

    float   val()   const { return val_; }
    float   min()   const { return val_min_; }
    float   max()   const { return val_max_; }
    float   dVdT()  const { return dVdT_; }

    std::string json() const;
};


