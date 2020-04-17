#pragma once

#include <string>
#include <chrono>
#include <limits>
#include <algorithm>

// record value, min, max, dV/dT, averaged dV/dT
//


// average value
template<typename T>
class average_t
{

public:
    average_t(size_t max_count, T init_val=0) : sum_(0), count_(0), max_count_(std::max(size_t(1),max_count))
	{
		add(init_val);
	};


	T add(const T& val)
	{
		T diff = get() - val;

		if(count_ == max_count_)
		{
			sum_ = get() * (max_count_-1) + val;
		}
		else
		{
			++count_;
			sum_ += val;
		}

		return diff;

	}

	T get() const
	{
		if(!count_)
			return sum_;
		return T(sum_)/count_;
	}

	// operator double() const { T val = get(); return val; }

	void reset(const T& val)
	{
		sum_ = val;
		count_ = 1;
	}

private:
	T 		sum_;
	size_t 	count_;
	size_t 	max_count_;

};


class dynamics_t
{
public:
    using tp = std::chrono::steady_clock::time_point;

private:
    bool    initialised_ = false;
    float   val_ = 0;   // updated on every add()
    float   val_prev_ = 0; // updated after min_dT_
    tp      val_prev_timestamp_; // timestamp of val_prev_ update
    float   val_min_ = std::numeric_limits<float>::max();
    float   val_max_ = -std::numeric_limits<float>::min();
    float   dVdT_ = 0;
    average_t<float> dVdT_avg_ = average_t<float>(5);

public:
    int     min_dT_ = 10; // minimum time difference to compute dV/dT. Sample too close to previous will be rejected

    bool    initialised() const { return initialised_; }

    void    add(const tp timestamp, const float val); // utc_seconds is HHMMSS converted to seconds. return true if sample was accepted

    float   val()   const { return val_; }
    float   min()   const { return val_min_; }
    float   max()   const { return val_max_; }
    float   dVdT()  const { return dVdT_; }
    float   dVdT_avg()  const { return dVdT_avg_.get(); }

    std::string str() const;
    std::string json() const;
};


