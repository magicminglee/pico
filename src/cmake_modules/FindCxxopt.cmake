find_path(Cxxopt_INCLUDE_DIRS
  NAMES cxxopts.hpp
  DOC "Cxxopt include dir"
  PATH_SUFFIXES cxxopt/include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cxxopt
  DEFAULT_MSG
  Cxxopt_INCLUDE_DIRS)

mark_as_advanced(Cxxopt_INCLUDE_DIRS)

if(Cxxopt_FOUND)
  if(NOT TARGET Cxxopt_lib)
    add_library(Cxxopt_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Cxxopt_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${Cxxopt_INCLUDE_DIRS}")
endif(Cxxopt_FOUND)