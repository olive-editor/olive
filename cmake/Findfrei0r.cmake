
# CMake module to search for frei0r
# Author: Rohit Yadav <rohityadav89@gmail.com>
#
# If it's found it sets FREI0R_FOUND to TRUE
# and following variables are set:
#    FREI0R_INCLUDE_DIR

# Put here path to custom location
# example: /home/username/frei0r/include etc..
find_path(FREI0R_INCLUDE_DIR NAMES frei0r.h
  PATHS
    "$ENV{LIB_DIR}/include"
    "/usr/include"
    "/usr/include/frei0r"
    "/usr/local/include"
    "/usr/local/include/frei0r"
    # Mac OS
    "${CMAKE_CURRENT_SOURCE_DIR}/contribs/include"
    # MingW
    c:/msys/local/include
)

find_path(FREI0R_INCLUDE_DIR PATHS "${CMAKE_INCLUDE_PATH}" NAMES frei0r.h)

# TODO: If required, add code to link to some library

if(FREI0R_INCLUDE_DIR)
  set(FREI0R_FOUND TRUE)
endif()

if(FREI0R_FOUND)
  if(NOT FREI0R_FIND_QUIETLY)
    message(STATUS "Found frei0r include-dir path: ${FREI0R_INCLUDE_DIR}")
  endif()
else()
  if(FREI0R_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find frei0r")
  elseif(NOT FREI0R_FIND_QUIETLY)
    message(STATUS "Could not find frei0r")
  endif()
endif()
