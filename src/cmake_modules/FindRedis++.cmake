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
find_path(Redis++_INCLUDE_DIRS
          NAMES sw/redis++/redis.hpp
          DOC "Redis++ include dir"
          PATH_SUFFIXES rediscxx/include)

find_library(Redis++_LIBRARIES NAMES redis++ DOC "redis++ library" PATH_SUFFIXES rediscxx/lib)

# handle the QUIETLY and REQUIRED arguments and set SASL2_FOUND to TRUE if all
# listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Redis++
                                  DEFAULT_MSG
                                  Redis++_INCLUDE_DIRS
                                  Redis++_LIBRARIES)
mark_as_advanced(Redis++_INCLUDE_DIRS Redis++_LIBRARIES)

if(Redis++_FOUND)
  if (NOT TARGET Redis++_lib)
    add_library(Redis++_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Redis++_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${Redis++_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${Redis++_LIBRARIES}")

endif(Redis++_FOUND)