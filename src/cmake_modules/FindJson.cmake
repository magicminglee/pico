find_path(JSON_INCLUDE_DIRS
  NAMES nlohmann/json.hpp
  DOC "json include dir"
  PATH_SUFFIXES json/include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Json
  DEFAULT_MSG
  JSON_INCLUDE_DIRS
)

mark_as_advanced(JSON_INCLUDE_DIRS)

if(Json_FOUND)
  if(NOT TARGET Json_lib)
    add_library(Json_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Json_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${JSON_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${JSON_LIBRARIES}")
endif(Json_FOUND)