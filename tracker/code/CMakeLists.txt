
cmake_minimum_required( VERSION 3.1 )

project (pizero_tracker)

set ( CMAKE_CXX_STANDARD 11 )
set ( CMAKE_BUILD_TYPE "Release" )

option( build_tests "build tests" 0 )

if( ${build_tests} )
	add_subdirectory( ublox )
	add_subdirectory( mtx2b )
endif()