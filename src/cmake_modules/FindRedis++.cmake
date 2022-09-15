find_path(Redis++_INCLUDE_DIRS
  NAMES sw/redis++/redis.hpp
  DOC "Redis++ include dir"
  PATH_SUFFIXES rediscxx/include)

find_library(Redis++_LIBRARIES NAMES redis++ DOC "redis++ library" PATH_SUFFIXES rediscxx/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Redis++
  DEFAULT_MSG
  Redis++_INCLUDE_DIRS
  Redis++_LIBRARIES)
mark_as_advanced(Redis++_INCLUDE_DIRS Redis++_LIBRARIES)

if(Redis++_FOUND)
  if(NOT TARGET Redis++_lib)
    add_library(Redis++_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Redis++_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${Redis++_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${Redis++_LIBRARIES}")
endif(Redis++_FOUND)