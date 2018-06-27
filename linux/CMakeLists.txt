cmake_minimum_required( VERSION 3.5.1 FATAL_ERROR )
include( FindPkgConfig )

project( n_rf24l01_linux ) 

option( DEBUG "To control the debug information generation" OFF )

# an spi_dev driver controls only work around an spi hardware and to control hardware pins a some way has to provided
option( SPI_DEV_BASED "The library is based on a Linux standart spi_dev kernel device driver" ON )

# generate a ${PROJECT_BINARY_DIR}/config.h file
configure_file( ${PROJECT_SOURCE_DIR}/config.h.in ${PROJECT_BINARY_DIR}/config.h )

# a config.h file is located in ${PROJECT_BINARY_DIR} so sources need a path to it
include_directories( ${PROJECT_BINARY_DIR} )

set( target "n_rf24l01" )

if( ${SPI_DEV_BASED} )
  set( target_src "src/linux_spi_dev/n_rf24l01.c" )
endif( ${SPI_DEV_BASED} )

add_library( ${target} SHARED ${target_src} )

target_compile_options( ${target} PUBLIC -g3 -o0 -Wall -fdebug-prefix-map=`pwd`=/home/odroid/n_rf24l01/libn_rf24l01 )
target_include_directories( ${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
                                             "${CMAKE_CURRENT_SOURCE_DIR}/.." )
