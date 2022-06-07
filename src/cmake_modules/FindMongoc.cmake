# Find mongoc
#
# Find the mongoc includes and library
#
# if you nee to add a custom library search path, do it via via
# CMAKE_PREFIX_PATH
#
# This module defines MONGOC_INCLUDE_DIRS, where to find header, etc.
# MONGOC_LIBRARIES, the libraries needed to use mongoc. MONGOC_FOUND, If
# false, do not try to use mongoc. 
# Mongoc_lib - The imported target library.

# only look in default directories
find_path(MONGOC_INCLUDE_DIRS
          NAMES mongoc.h
          DOC "mongoc include dir"
          PATH_SUFFIXES mongoc/include/libmongoc-1.0)

find_library(MONGOC_LIBRARIES NAMES mongoc-1.0 DOC "mongoc library" PATH_SUFFIXES mongoc/lib)

# handle the QUIETLY and REQUIRED arguments and set MONGOC_FOUND to TRUE if all
# listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mongoc
                                  DEFAULT_MSG
                                  MONGOC_INCLUDE_DIRS
                                  MONGOC_LIBRARIES)
mark_as_advanced(MONGOC_INCLUDE_DIRS MONGOC_LIBRARIES)

if(Mongoc_FOUND)
  if (NOT TARGET Mongoc_lib)
    add_library(Mongoc_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Mongoc_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${MONGOC_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${MONGOC_LIBRARIES}")

endif(Mongoc_FOUND)

find_path(BSON_INCLUDE_DIRS
          NAMES bson.h
          DOC "bson include dir"
          PATH_SUFFIXES mongoc/include/libbson-1.0)

find_library(BSON_LIBRARIES NAMES bson-1.0 DOC "bson library" PATH_SUFFIXES mongoc/lib)

# handle the QUIETLY and REQUIRED arguments and set MONGOC_FOUND to TRUE if all
# listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bson
                                  DEFAULT_MSG
                                  BSON_INCLUDE_DIRS
                                  BSON_LIBRARIES)
mark_as_advanced(BSON_INCLUDE_DIRS BSON_LIBRARIES)

if(Bson_FOUND)
  if (NOT TARGET Bson_lib)
    add_library(Bson_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Bson_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${BSON_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${BSON_LIBRARIES}")

endif(Bson_FOUND)