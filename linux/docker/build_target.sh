#!/bin/bash

# this script is supposed to be called from CONTAINER !!!
# do NOT call it directly from the host !!!

# Note: containers have to set up at least the next environment variables:
#       TARGET_NAME  - name of a target (e.g. arm-linux-gnueabihf)
#       CMAKE_TOOLCHAIN_FILE - a toolchain file for cross-compilation with cmake,
#                              if any cross-compilation is supposed

# at least a protect against being launched on the host machine 
if [ x${TARGET_NAME} == x ]; then
  echo "TARGET_NAME env variable isn't set."
  exit
fi

# it's supposed that we're in /home/dev/build directory

# within the container a CMAKE_TOOLCHAIN_FILE env variable points to the
# toolchain.cmake file which instructs cmake where to look for compilers,
# linkers..., includes, libraries... and so on
#
# /home/dev/project/src - is where CMakeLists.txt placed.
cmake -DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE -DDEBUG=ON -DTESTS=ON /home/dev/project/linux/
#make VERBOSE=1
make

if [ $? == 0 ]; then
  echo -e "\nBuild for ${TARGET_NAME} has been successfully finished."
fi
