# Find sasl2
#
# Find the sasl2 includes and library
#
# if you nee to add a custom library search path, do it via via
# CMAKE_PREFIX_PATH
#
# This module defines SASL2_INCLUDE_DIRS, where to find header, etc.
# SASL2_LIBRARIES, the libraries needed to use sasl2. SASL2_FOUND, If
# false, do not try to use sasl2. 
# Sasl2_lib - The imported target library.

# only look in default directories
find_path(LIBEVENT_INCLUDE_DIRS
          NAMES event.h
          DOC "libevent include dir"
          PATH_SUFFIXES libevent/include)

find_library(event_LIBRARIES NAMES event DOC "libevent library" PATH_SUFFIXES libevent/lib)
find_library(event_openssl_LIBRARIES NAMES event_openssl DOC "libevent library" PATH_SUFFIXES libevent/lib)

# handle the QUIETLY and REQUIRED arguments and set SASL2_FOUND to TRUE if all
# listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libevent
                                  DEFAULT_MSG
                                  LIBEVENT_INCLUDE_DIRS
                                  event_LIBRARIES
                                  event_openssl_LIBRARIES)
mark_as_advanced(LIBEVENT_INCLUDE_DIRS event_LIBRARIES event_openssl_LIBRARIES)

if(Libevent_FOUND)
  if (NOT TARGET Libevent_lib)
    add_library(Libevent_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Libevent_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${LIBEVENT_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${event_LIBRARIES};${event_openssl_LIBRARIES}")

endif(Libevent_FOUND)