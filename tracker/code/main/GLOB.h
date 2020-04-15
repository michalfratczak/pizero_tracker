#pragma once

#include <string>
#include <vector>
#include <map>
#include <atomic>
#include <chrono>

#include "mtx2/mtx2.h"
#include "nmea/nmea.h"
#include "dynamics_t.h"


// global options - singleton

class GLOB
{

private:
    GLOB() {};
    GLOB( const GLOB& ) = delete; // non construction-copyable
    GLOB& operator=( const GLOB& ) = delete; // non copyable

    std::atomic<nmea_t> nmea_; // GPS data
    std::chrono::steady_clock::time_point gps_fix_timestamp_;

    // sensors dynamics
    std::map<std::string, dynamics_t>    dynamics_; // index: value name (alt, temp1, etc.)

public:
    static GLOB& get()
    {
        static GLOB g;
        return g;
    }

    struct cli_t // command line interface options
    {
        std::string     callsign;
        float           freqMHz = 0;    //MegaHertz
        baud_t          baud = baud_t::kInvalid;
        std::string     ssdv_image;     // ssdv encoded image path
        int             msg_num = 5;    // number of telemetry sentences emitted between SSDV packets
        int             port = 6666;    // zeroMQ server port

        // hardware config
        int             hw_pin_radio_on = 0;    // gpio numbered pin for radio enable. current board: 22
        std::string     hw_radio_serial;        // serial device for MTX2 radio. for rPI4: /dev/serial0
        std::string     hw_ublox_device;        // I2C device for uBLOX. for rPI4: /dev/i2c-7
    };
    cli_t cli;

    bool        dynamics_add(const std::string& name, const dynamics_t::tp timestamp, const float value);
    dynamics_t  dynamics_get(const std::string& name) const;
    std::vector<std::string>  dynamics_keys() const; // names in dynamics_

    void    nmea_set(const nmea_t& in_nmea) { get().nmea_ = in_nmea; }
    nmea_t  nmea_get() { nmea_t ret = get().nmea_; return ret; }

    void gps_fix_now() { gps_fix_timestamp_ = std::chrono::steady_clock::now(); }
    int  gps_fix_age() const { return (std::chrono::steady_clock::now() - gps_fix_timestamp_).count() / 1e9; }

    std::string str() const;

};
