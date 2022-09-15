find_path(nghttp2_INCLUDE_DIRS
  NAMES nghttp2/nghttp2.h
  DOC "nghttps include dir"
  PATH_SUFFIXES nghttp2/include)

find_library(nghttp2_LIBRARIES NAMES nghttp2 DOC "nghttp2 library" PATH_SUFFIXES nghttp2/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Nghttp2
  DEFAULT_MSG
  nghttp2_INCLUDE_DIRS
  nghttp2_LIBRARIES)

mark_as_advanced(nghttp2_INCLUDE_DIRS nghttp2_LIBRARIES)

if(Nghttp2_FOUND)
  if(NOT TARGET Nghttp2_lib)
    add_library(Nghttp2_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Nghttp2_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${nghttp2_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${nghttp2_LIBRARIES}")
endif(Nghttp2_FOUND)