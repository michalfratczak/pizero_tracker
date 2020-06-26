#include <iostream>


struct gps_distance_t
{
    double   dist_line_ = 0;
    double   dist_circle_ = 0;
    double   dist_radians_ = 0;  // angular distance over Earth surface, in radians
    double   elevation_ = 0;     // degrees
    double   bearing_ = 0;       // degrees
};

std::ostream& operator<<(std::ostream& o, const gps_distance_t& rhs );

gps_distance_t calc_gps_distance(double lat1, double lon1, double alt1, double lat2, double lon2, double alt2);
