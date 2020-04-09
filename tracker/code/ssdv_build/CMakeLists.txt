
set (ssdv_src
    ${CMAKE_CURRENT_SOURCE_DIR}/../ssdv/rs8.h   ${CMAKE_CURRENT_SOURCE_DIR}/../ssdv/rs8.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../ssdv/ssdv.h   ${CMAKE_CURRENT_SOURCE_DIR}/../ssdv/ssdv.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../ssdv/main.c
)

add_executable( ssdv ${ssdv_src} )
