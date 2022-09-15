find_path(SASL2_INCLUDE_DIRS
  NAMES sasl/sasl.h
  DOC "sasl2 include dir"
  PATH_SUFFIXES sasl2/include)

find_library(SASL2_LIBRARIES NAMES sasl2 DOC "sasl2 library" PATH_SUFFIXES sasl2/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sasl2
  DEFAULT_MSG
  SASL2_INCLUDE_DIRS
  SASL2_LIBRARIES)
mark_as_advanced(SASL2_INCLUDE_DIRS SASL2_LIBRARIES)

if(Sasl2_FOUND)
  if(NOT TARGET Sasl2_lib)
    add_library(Sasl2_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Sasl2_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${SASL2_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${SASL2_LIBRARIES}")
endif(Sasl2_FOUND)