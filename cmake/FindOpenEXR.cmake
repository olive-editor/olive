# Module to find OpenEXR and Imath.
#
# I'm afraid this is a mess, due to needing to support a wide range of
# OpenEXR versions.
#
# For OpenEXR & Imath 3.0, this will establish the following imported
# targets:
#
#    Imath::Imath
#    Imath::Half
#    OpenEXR::OpenEXR
#    OpenEXR::Iex
#    OpenEXR::IlmThread
#
# For OpenEXR 2.4 & 2.5, it will establish the following imported targets:
#
#    IlmBase::Imath
#    IlmBase::Half
#    IlmBase::Iex
#    IlmBase::IlmThread
#    OpenEXR::IlmImf
#
# For all versions -- but for OpenEXR < 2.4 the only thing this sets --
# are the following CMake variables:
#
#   OPENEXR_FOUND          true, if found
#   OPENEXR_INCLUDES       directory where OpenEXR headers are found
#   OPENEXR_LIBRARIES      libraries for OpenEXR + IlmBase
#   OPENEXR_VERSION        OpenEXR version (accurate for >= 2.0.0,
#                              otherwise will just guess 1.6.1)
#   IMATH_INCLUDES         directory where Imath headers are found
#   ILMBASE_INCLUDES       directory where IlmBase headers are found
#   ILMBASE_LIBRARIES      libraries just IlmBase
#
#

# First, try to fine just the right config files
find_package(Imath CONFIG)
if (NOT TARGET Imath::Imath)
    # Couldn't find Imath::Imath, maybe it's older and has IlmBase?
    find_package(IlmBase CONFIG)
endif ()
find_package(OpenEXR CONFIG)

if (TARGET OpenEXR::OpenEXR AND TARGET Imath::Imath)
    # OpenEXR 3.x if both of these targets are found
    set (FOUND_OPENEXR_WITH_CONFIG 1)
    if (NOT OpenEXR_FIND_QUIETLY)
        message (STATUS "Found CONFIG for OpenEXR 3 (OPENEXR_VERSION=${OpenEXR_VERSION})")
    endif ()

    # Mimic old style variables
    set (OPENEXR_VERSION ${OpenEXR_VERSION})
    get_target_property(IMATH_INCLUDES Imath::Imath INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(ILMBASE_INCLUDES Imath::Imath INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(ILMBASE_IMATH_LIBRARY Imath::Imath INTERFACE_LINK_LIBRARIES)
    get_target_property(IMATH_LIBRARY Imath::Imath INTERFACE_LINK_LIBRARIES)
    get_target_property(OPENEXR_IEX_LIBRARY OpenEXR::Iex INTERFACE_LINK_LIBRARIES)
    get_target_property(OPENEXR_ILMTHREAD_LIBRARY OpenEXR::IlmThread INTERFACE_LINK_LIBRARIES)
    set (ILMBASE_LIBRARIES ${ILMBASE_IMATH_LIBRARY})
    set (ILMBASE_FOUND true)

    get_target_property(OPENEXR_INCLUDES OpenEXR::OpenEXR INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(OPENEXR_ILMIMF_LIBRARY OpenEXR::OpenEXR INTERFACE_LINK_LIBRARIES)
    set (OPENEXR_LIBRARIES OpenEXR::OpenEXR ${OPENEXR_ILMIMF_LIBRARY} ${OPENEXR_IEX_LIBRARY} ${OPENEXR_ILMTHREAD_LIBRARY} ${ILMBASE_LIBRARIES})
    set (OPENEXR_FOUND true)

    # Link with pthreads if required
    find_package (Threads)
    if (CMAKE_USE_PTHREADS_INIT)
        list (APPEND ILMBASE_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
    endif ()

elseif (TARGET OpenEXR::IlmImf AND TARGET IlmBase::Imath AND
        (OPENEXR_VERSION VERSION_GREATER_EQUAL 2.4 OR OpenEXR_VERSION VERSION_GREATER_EQUAL 2.4))
    # OpenEXR 2.4 or 2.5 with exported config
    set (FOUND_OPENEXR_WITH_CONFIG 1)
    if (NOT OpenEXR_FIND_QUIETLY)
        message (STATUS "Found CONFIG for OpenEXR 2 (OPENEXR_VERSION=${OpenEXR_VERSION})")
    endif ()

    # Mimic old style variables
    get_target_property(ILMBASE_INCLUDES IlmBase::IlmBaseConfig INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(IMATH_INCLUDES IlmBase::IlmBaseConfig INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(ILMBASE_Imath_LIBRARY IlmBase::Imath INTERFACE_LINK_LIBRARIES)
    get_target_property(ILMBASE_IMATH_LIBRARY IlmBase::Imath INTERFACE_LINK_LIBRARIES)
    get_target_property(ILMBASE_HALF_LIBRARY IlmBase::Half INTERFACE_LINK_LIBRARIES)
    get_target_property(OPENEXR_IEX_LIBRARY IlmBase::Iex INTERFACE_LINK_LIBRARIES)
    get_target_property(OPENEXR_ILMTHREAD_LIBRARY IlmBase::IlmThread INTERFACE_LINK_LIBRARIES)
    set (ILMBASE_LIBRARIES ${ILMBASE_IMATH_LIBRARY} ${ILMBASE_HALF_LIBRARY} ${OPENEXR_IEX_LIBRARY} ${OPENEXR_ILMTHREAD_LIBRARY})
    set (ILMBASE_FOUND true)

    get_target_property(OPENEXR_INCLUDES OpenEXR::IlmImfConfig INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(OPENEXR_ILMIMF_LIBRARY OpenEXR::IlmImf INTERFACE_LINK_LIBRARIES)
    set (OPENEXR_LIBRARIES OpenEXR::IlmImf ${OPENEXR_ILMIMF_LIBRARY} ${ILMBASE_LIBRARIES})
    set (OPENEXR_FOUND true)

    # Link with pthreads if required
    find_package (Threads)
    if (CMAKE_USE_PTHREADS_INIT)
        list (APPEND ILMBASE_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})
    endif ()

    # Correct for how old OpenEXR config exports set the directory one
    # level lower than we prefer it.
    string(REGEX REPLACE "include/OpenEXR$" "include" ILMBASE_INCLUDES "${ILMBASE_INCLUDES}")
    string(REGEX REPLACE "include/OpenEXR$" "include" IMATH_INCLUDES "${IMATH_INCLUDES}")
    string(REGEX REPLACE "include/OpenEXR$" "include" OPENEXR_INCLUDES "${OPENEXR_INCLUDES}")

else ()
    # OpenEXR 2.x older versions without a config or whose configs we don't
    # trust.

    set (FOUND_OPENEXR_WITH_CONFIG 0)

# Other standard issue macros
include (FindPackageHandleStandardArgs)
include (SelectLibraryConfigurations)

find_package (ZLIB REQUIRED)

# Link with pthreads if required
find_package (Threads)
if (CMAKE_USE_PTHREADS_INIT)
    set (ILMBASE_PTHREADS ${CMAKE_THREAD_LIBS_INIT})
endif ()

# Attempt to find OpenEXR with pkgconfig
find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    if (NOT Ilmbase_ROOT AND NOT ILMBASE_ROOT
        AND NOT DEFINED ENV{Ilmbase_ROOT} AND NOT DEFINED ENV{ILMBASE_ROOT})
        pkg_check_modules(_ILMBASE QUIET IlmBase>=2.0.0)
    endif ()
    if (NOT OpenEXR_ROOT AND NOT OPENEXR_ROOT
        AND NOT DEFINED ENV{OpenEXR_ROOT} AND NOT DEFINED ENV{OPENEXR_ROOT})
        pkg_check_modules(_OPENEXR QUIET OpenEXR>=2.0.0)
    endif ()
endif (PKG_CONFIG_FOUND)

# List of likely places to find the headers -- note priority override of
# ${OPENEXR_ROOT}/include.
# ILMBASE is needed in case ilmbase an openexr are installed in separate
# directories, like NixOS does
set (GENERIC_INCLUDE_PATHS
    ${OPENEXR_ROOT}/include
    $ENV{OPENEXR_ROOT}/include
    ${ILMBASE_ROOT}/include
    $ENV{ILMBASE_ROOT}/include
    ${_ILMBASE_INCLUDEDIR}
    ${_OPENEXR_INCLUDEDIR}
    /usr/local/include
    /usr/include
    /usr/include/${CMAKE_LIBRARY_ARCHITECTURE}
    /sw/include
    /opt/local/include )

# Find the include file locations.
find_path (ILMBASE_INCLUDE_PATH OpenEXR/IlmBaseConfig.h
           HINTS ${ILMBASE_INCLUDE_DIR} ${OPENEXR_INCLUDE_DIR}
                 ${GENERIC_INCLUDE_PATHS} )
find_path (OPENEXR_INCLUDE_PATH OpenEXR/OpenEXRConfig.h
           HINTS ${OPENEXR_INCLUDE_DIR}
                 ${GENERIC_INCLUDE_PATHS} )

# Try to figure out version number
if (DEFINED _OPENEXR_VERSION AND NOT "${_OPENEXR_VERSION}" STREQUAL "")
    set (OPENEXR_VERSION "${_OPENEXR_VERSION}")
    string (REGEX REPLACE "([0-9]+)\\.[0-9\\.]+" "\\1" OPENEXR_VERSION_MAJOR "${_OPENEXR_VERSION}")
    string (REGEX REPLACE "[0-9]+\\.([0-9]+)(\\.[0-9]+)?" "\\1" OPENEXR_VERSION_MINOR "${_OPENEXR_VERSION}")
elseif (EXISTS "${OPENEXR_INCLUDE_PATH}/OpenEXR/ImfMultiPartInputFile.h")
    # Must be at least 2.0
    file(STRINGS "${OPENEXR_INCLUDE_PATH}/OpenEXR/OpenEXRConfig.h" TMP REGEX "^#define OPENEXR_VERSION_STRING .*$")
    string (REGEX MATCHALL "[0-9]+[.0-9]+" OPENEXR_VERSION ${TMP})
    file(STRINGS "${OPENEXR_INCLUDE_PATH}/OpenEXR/OpenEXRConfig.h" TMP REGEX "^#define OPENEXR_VERSION_MAJOR .*$")
    string (REGEX MATCHALL "[0-9]+" OPENEXR_VERSION_MAJOR ${TMP})
    file(STRINGS "${OPENEXR_INCLUDE_PATH}/OpenEXR/OpenEXRConfig.h" TMP REGEX "^#define OPENEXR_VERSION_MINOR .*$")
    string (REGEX MATCHALL "[0-9]+" OPENEXR_VERSION_MINOR ${TMP})
else ()
    # Assume an old one, predates 2.x that had versions
    set (OPENEXR_VERSION 1.6.1)
    set (OPENEXR_MAJOR 1)
    set (OPENEXR_MINOR 6)
endif ()


# List of likely places to find the libraries -- note priority override of
# ${OPENEXR_ROOT}/lib.
set (GENERIC_LIBRARY_PATHS
    ${OPENEXR_ROOT}/lib
    ${ILMBASE_ROOT}/lib
    ${OPENEXR_INCLUDE_PATH}/../lib
    ${ILMBASE_INCLUDE_PATH}/../lib
    ${_ILMBASE_LIBDIR}
    ${_OPENEXR_LIBDIR}
    /usr/local/lib
    /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
    /usr/lib
    /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
    /sw/lib
    /opt/local/lib
    $ENV{PROGRAM_FILES}/OpenEXR/lib/static )

# message (STATUS "Generic lib paths: ${GENERIC_LIBRARY_PATHS}")

# Handle request for static libs by altering CMAKE_FIND_LIBRARY_SUFFIXES.
# We will restore it at the end of this file.
set (_openexr_orig_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
if (OpenEXR_USE_STATIC_LIBS)
    if (WIN32)
        set (CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else ()
        set (CMAKE_FIND_LIBRARY_SUFFIXES .a)
    endif ()
endif ()

# Look for the libraries themselves, for all the components.
# This is complicated because the OpenEXR libraries may or may not be
# built with version numbers embedded.
set (_openexr_components IlmThread IlmImf Imath Iex Half)
foreach (COMPONENT ${_openexr_components})
    string (TOUPPER ${COMPONENT} UPPERCOMPONENT)
    # First try with the version embedded
    find_library (OPENEXR_${UPPERCOMPONENT}_LIBRARY
                  NAMES ${COMPONENT}-${OPENEXR_VERSION_MAJOR}_${OPENEXR_VERSION_MINOR}
                        ${COMPONENT}
                        ${COMPONENT}-${OPENEXR_VERSION_MAJOR}_${OPENEXR_VERSION_MINOR}_d
                        ${COMPONENT}_d
                  HINTS ${OPENEXR_LIBRARY_DIR} $ENV{OPENEXR_LIBRARY_DIR}
                        ${GENERIC_LIBRARY_PATHS} )
endforeach ()

find_package_handle_standard_args (OpenEXR
    REQUIRED_VARS ILMBASE_INCLUDE_PATH OPENEXR_INCLUDE_PATH
                  OPENEXR_IMATH_LIBRARY OPENEXR_ILMIMF_LIBRARY
                  OPENEXR_IEX_LIBRARY OPENEXR_HALF_LIBRARY
    )

if (OPENEXR_FOUND)
    set (OpenEXR_VERSION ${OPENEXR_VERSION})
    set (ILMBASE_FOUND TRUE)
    set (ILMBASE_INCLUDES ${ILMBASE_INCLUDE_PATH})
    set (IMATH_INCLUDES ${ILMBASE_INCLUDE_PATH})
    set (OPENEXR_INCLUDES ${OPENEXR_INCLUDE_PATH})
    set (ILMBASE_INCLUDE_DIR ${ILMBASE_INCLUDE_PATH})
    set (IMATH_INCLUDE_DIR ${ILMBASE_INCLUDE_PATH})
    set (OPENEXR_INCLUDE_DIR ${OPENEXR_INCLUDE_PATH})
    set (ILMBASE_LIBRARIES ${OPENEXR_IMATH_LIBRARY} ${OPENEXR_IEX_LIBRARY} ${OPENEXR_HALF_LIBRARY} ${OPENEXR_ILMTHREAD_LIBRARY} ${ILMBASE_PTHREADS} CACHE STRING "The libraries needed to use IlmBase")
    set (OPENEXR_LIBRARIES ${OPENEXR_ILMIMF_LIBRARY} ${ILMBASE_LIBRARIES} ${ZLIB_LIBRARIES} CACHE STRING "The libraries needed to use OpenEXR")
    set (FINDOPENEXR_FALLBACK TRUE)
endif ()

mark_as_advanced(
    OPENEXR_ILMIMF_LIBRARY
    OPENEXR_IMATH_LIBRARY
    OPENEXR_IEX_LIBRARY
    OPENEXR_HALF_LIBRARY
    OPENEXR_VERSION)

# Restore the original CMAKE_FIND_LIBRARY_SUFFIXES
set (CMAKE_FIND_LIBRARY_SUFFIXES ${_openexr_orig_suffixes})

endif ()
