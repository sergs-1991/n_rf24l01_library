# look to {CMAKE_ROOT}/Modules/Platform/Linux.cmake file
set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_SYSTEM_VERSION 1 )

# set toolchain name
set( tool_chain "arm-linux-gnueabihf" )

set( CMAKE_C_COMPILER   /usr/bin/${tool_chain}-gcc )
set( CMAKE_CXX_COMPILER /usr/bin/${tool_chain}-g++ )

# set path for the target environment, this path will be prepended before /lib, /usr/lib, ...,
# and FIND_XXX macros will look for under these prepended paths firstly and then under
# /lib, /usr/lib, ...
set( CMAKE_FIND_ROOT_PATH /usr/${tool_chain} )

# disable searching programms in CMAKE_FIND_ROOT_PATH for all FIND_PROGRAM macros
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )

# allow searching libraries and headers(files, paths) only within CMAKE_FIND_ROOT_PATH,
# for all FIND_LIBRARY, FIND_FILES and FIND_PATH macros
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

# NOTE: we can redefine these behaviour individually for each FIND_XXX calls
#       (NO_CMAKE_FIND_ROOT_PATH, ONLY_CMAKE_FIND_ROOT_PATH and CMAKE_FIND_ROOT_PATH_BOTH)
