
set (ublox_test_i2c_src
	ublox_cmds.h ublox_cmds.cpp
	ublox.h ublox.cpp
	../nmea/nmea.h ../nmea/nmea.cpp
	ublox_test_i2c.cpp
)

include_directories( ${CMAKE_SOURCE_DIR} )

add_executable( ublox_test_i2c ${ublox_test_i2c_src})