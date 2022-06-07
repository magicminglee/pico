# Find mongocxx
#
# Find the mongocxx includes and library
#
# if you nee to add a custom library search path, do it via via
# CMAKE_PREFIX_PATH
#
# This module defines MONGOCXX_INCLUDE_DIRS, where to find header, etc.
# MONGOCXX_LIBRARIES, the libraries needed to use mongocxx. MONGOCXX_FOUND, If
# false, do not try to use mongocxx. 
# Mongoc_lib - The imported target library.

# only look in default directories
find_path(JSON_INCLUDE_DIRS
          NAMES nlohmann/json.hpp
          DOC "json include dir"
          PATH_SUFFIXES json/include)

# handle the QUIETLY and REQUIRED arguments and set MONGOCXX_FOUND to TRUE if all
# listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Json
                                  DEFAULT_MSG
                                  JSON_INCLUDE_DIRS
                                  )
mark_as_advanced(JSON_INCLUDE_DIRS)

if(Json_FOUND)
  if (NOT TARGET Json_lib)
    add_library(Json_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Json_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${JSON_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${JSON_LIBRARIES}")

endif(Json_FOUND)