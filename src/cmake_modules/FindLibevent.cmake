find_path(LIBEVENT_INCLUDE_DIRS
  NAMES event.h
  DOC "libevent include dir"
  PATH_SUFFIXES libevent/include)

find_library(event_LIBRARIES NAMES event DOC "libevent library" PATH_SUFFIXES libevent/lib)
find_library(event_openssl_LIBRARIES NAMES event_openssl DOC "libevent library" PATH_SUFFIXES libevent/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libevent
  DEFAULT_MSG
  LIBEVENT_INCLUDE_DIRS
  event_LIBRARIES
  event_openssl_LIBRARIES)

mark_as_advanced(LIBEVENT_INCLUDE_DIRS event_LIBRARIES event_openssl_LIBRARIES)

if(Libevent_FOUND)
  if(NOT TARGET Libevent_lib)
    add_library(Libevent_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Libevent_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${LIBEVENT_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${event_LIBRARIES};${event_openssl_LIBRARIES}")
endif(Libevent_FOUND)