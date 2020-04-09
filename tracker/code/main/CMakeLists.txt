

set ( Boost_USE_STATIC_LIBS ON )
find_package(Boost REQUIRED COMPONENTS program_options )
include_directories( ${Boost_INCLUDE_DIRS} )

set (tracker_src
	GLOB.h GLOB.cpp
	cli.h cli.cpp
	ssdv_t.cpp
	main.cpp
	../mtx2/mtx2.cpp
	../nmea/nmea.cpp
	../ublox/ublox_cmds.cpp
	../ublox/ublox.cpp
	../ds18b20/ds18b20.cpp
)

include_directories( ${CMAKE_SOURCE_DIR} )

add_executable( tracker ${tracker_src})

target_link_libraries( tracker "pigpio" ${Boost_LIBRARIES} )
