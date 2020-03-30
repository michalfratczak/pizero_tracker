
set (mtx2b_test_src
	mtx2b.h mtx2b.cpp
	mtx2b_test.cpp
)

include_directories( ${CMAKE_SOURCE_DIR} )

add_executable( mtx2b_test ${mtx2b_test_src})