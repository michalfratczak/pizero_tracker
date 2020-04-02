
set (nmea_test_src
    nmea.cpp
    nmea_test.cpp
)

include_directories( ${CMAKE_SOURCE_DIR} )

add_executable( nmea_test ${nmea_test_src})
