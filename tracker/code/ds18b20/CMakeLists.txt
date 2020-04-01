
# req. by stdc++fs
CMAKE_POLICY(SET CMP0079 NEW)

set (ds18b20_test_src
	ds18b20.h ds18b20.cpp
	ds18b20_test.cpp
)

include_directories( ${CMAKE_SOURCE_DIR} )

add_executable( ds18b20_test ${ds18b20_test_src} )
