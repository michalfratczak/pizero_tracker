
set (tracker_src
	main.cpp
	ssdv_t.cpp
	../mtx2/mtx2.cpp
	../nmea/nmea.cpp
	../ublox/ublox_cmds.cpp
	../ublox/ublox.cpp
	../ds18b20/ds18b20.cpp
)

include_directories( ${CMAKE_SOURCE_DIR} )

add_executable( tracker ${tracker_src})

target_link_libraries( tracker "pigpio" )
