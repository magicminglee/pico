find_path(Jwtcxx_INCLUDE_DIRS
  NAMES jwt-cpp/jwt.h
  DOC "json include dir"
  PATH_SUFFIXES jwt-cpp/include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Jwtcxx
  DEFAULT_MSG
  Jwtcxx_INCLUDE_DIRS
)

mark_as_advanced(Jwtcxx_INCLUDE_DIRS)

if(Jwtcxx_FOUND)
  if(NOT TARGET Jwtcxx_lib)
    add_library(Jwtcxx_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Jwtcxx_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${Jwtcxx_INCLUDE_DIRS}")
endif(Jwtcxx_FOUND)