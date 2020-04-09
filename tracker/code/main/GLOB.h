#pragma once

#include <string>
#include <atomic>

#include "mtx2/mtx2.h"
#include "nmea/nmea.h"

// global options - singleton

class GLOB
{
private:
    GLOB() {};
    GLOB( const GLOB& ) = delete; // non construction-copyable
    GLOB& operator=( const GLOB& ) = delete; // non copyable

public:
    static GLOB& get()
    {
        static GLOB g;
        return g;
    }

    struct cli_t // command line interface options
    {
        std::string     callsign;
        float   freqMHz = 0; //MegaHertz
        baud_t  baud = baud_t::kInvalid;
        std::string     ssdv_image; // ssdv encoded image path


        // hardware config
        int             hw_pin_radio_on = 0;    // gpio numbered pin for radio enable. current board: 22
        std::string     hw_radio_serial;        // serial device for MTX2 radio. for rPI4: /dev/serial0
        std::string     hw_ublox_device;        // I2C device for uBLOX. for rPI4: /dev/i2c-7
    };

    cli_t cli;
    std::atomic<nmea_t> nmea; // GPS data

    std::string str() const;

};