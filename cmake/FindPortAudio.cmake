# - Find PortAudio includes and libraries
#
#   PORTAUDIO_FOUND        - True if PORTAUDIO_INCLUDE_DIR & PORTAUDIO_LIBRARY
#                            are found
#   PORTAUDIO_LIBRARIES    - Set when PORTAUDIO_LIBRARY is found
#   PORTAUDIO_INCLUDE_DIRS - Set when PORTAUDIO_INCLUDE_DIR is found
#
#   PORTAUDIO_INCLUDE_DIR - where to find portaudio.h, etc.
#   PORTAUDIO_LIBRARY     - the portaudio library
#

find_path(PORTAUDIO_INCLUDE_DIR
          NAMES portaudio.h
          DOC "The PortAudio include directory"
)

find_library(PORTAUDIO_LIBRARY
             NAMES portaudio
             DOC "The PortAudio library"
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PortAudio
    REQUIRED_VARS PORTAUDIO_LIBRARY PORTAUDIO_INCLUDE_DIR
)

if(PORTAUDIO_FOUND)
    set(PORTAUDIO_LIBRARIES ${PORTAUDIO_LIBRARY})
    set(PORTAUDIO_INCLUDE_DIRS ${PORTAUDIO_INCLUDE_DIR})
endif()

mark_as_advanced(PORTAUDIO_INCLUDE_DIR PORTAUDIO_LIBRARY)
