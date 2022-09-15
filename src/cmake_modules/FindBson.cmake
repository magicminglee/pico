find_path(BSON_INCLUDE_DIRS
  NAMES bson.h
  DOC "bson include dir"
  PATH_SUFFIXES mongoc/include/libbson-1.0)

find_library(BSON_LIBRARIES NAMES bson-1.0 DOC "bson library" PATH_SUFFIXES mongoc/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bson
  DEFAULT_MSG
  BSON_INCLUDE_DIRS
  BSON_LIBRARIES)
mark_as_advanced(BSON_INCLUDE_DIRS BSON_LIBRARIES)

if(Bson_FOUND)
  if(NOT TARGET Bson_lib)
    add_library(Bson_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Bson_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${BSON_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${BSON_LIBRARIES}")
endif(Bson_FOUND)