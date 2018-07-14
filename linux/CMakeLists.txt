cmake_minimum_required( VERSION 3.5.1 FATAL_ERROR )
include( FindPkgConfig )

project( n_rf24l01_linux ) 

option( DEBUG "To control the debug information generation" OFF )

option( SPI_DEV_BASED "The library is based on a Linux standart spi_dev kernel device driver" ON )

if( SPI_DEV_BASED )
  set( SPI_DEVICE_FILE "/dev/spidev1.0" )
  set( INTERRUPT_LINE_PIN_NUM 200 ) # on the odroid-u3 - J4(IO-Port#1) #200 pin
  set( CE_LINE_PIN_NUM 199 )        # on the odroid-u3 - J4(IO-Port#1) #199 pin
endif()

# generate a ${PROJECT_BINARY_DIR}/config.h file
configure_file( ${PROJECT_SOURCE_DIR}/config.h.in ${PROJECT_BINARY_DIR}/config.h )

# a config.h file is located in ${PROJECT_BINARY_DIR} so sources need a path to it
include_directories( ${PROJECT_BINARY_DIR} )

set( target "n_rf24l01" )

if( ${SPI_DEV_BASED} )
  set( wrap_back_src "src/linux_spi_dev/n_rf24l01.c" "src/linux_spi_dev/n_rf24l01_backend.c" )
endif( ${SPI_DEV_BASED} )

set( core_src "../core/n_rf24l01.c" )

add_library( ${target} SHARED ${core_src} ${wrap_back_src} )

target_compile_options( ${target} PUBLIC -g3 -o0 -Wall -fdebug-prefix-map=`pwd`=/home/odroid/n_rf24l01/libn_rf24l01 )
target_include_directories( ${target} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
                                             "${CMAKE_CURRENT_SOURCE_DIR}/.." )
if( ${SPI_DEV_BASED} )
  target_link_libraries( ${target} -pthread )
endif( ${SPI_DEV_BASED} )
