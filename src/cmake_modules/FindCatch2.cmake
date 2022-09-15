find_path(Catch2_INCLUDE_DIRS
  NAMES catch2/catch_config.hpp
  DOC "Catch2 include dir"
  PATH_SUFFIXES catch2/include)

find_library(Catch2_LIBRARIES NAMES Catch2 Catch2Main DOC "Catch2 library" PATH_SUFFIXES catch2/lib)
find_library(Catch2Main_LIBRARIES NAMES Catch2Main DOC "Catch2Main library" PATH_SUFFIXES catch2/lib)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Catch2
  DEFAULT_MSG
  Catch2_INCLUDE_DIRS
  Catch2_LIBRARIES
  Catch2Main_LIBRARIES)

mark_as_advanced(Catch2_INCLUDE_DIRS Catch2_LIBRARIES)

if(Catch2_FOUND)
  if(NOT TARGET Catch2_lib)
    add_library(Catch2_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Catch2_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${Catch2_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${Catch2_LIBRARIES};${Catch2Main_LIBRARIES}")
endif(Catch2_FOUND)