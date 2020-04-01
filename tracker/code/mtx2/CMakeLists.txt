
set (mtx2_test_src
	mtx2.h mtx2.cpp
	mtx2_test.cpp
)

include_directories( ${CMAKE_SOURCE_DIR} )

add_executable( mtx2_test ${mtx2_test_src})

target_link_libraries( mtx2_test "pigpio" )
