find_path(Hiredis_INCLUDE_DIRS
  NAMES hiredis/hiredis.h
  DOC "Hiredis include dir"
  PATH_SUFFIXES hiredis/include)

find_library(Hiredis_LIBRARIES NAMES hiredis DOC "Hiredis library" PATH_SUFFIXES hiredis/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Hiredis
  DEFAULT_MSG
  Hiredis_INCLUDE_DIRS
  Hiredis_LIBRARIES)

mark_as_advanced(Hiredis_INCLUDE_DIRS Hiredis_LIBRARIES)

if(Hiredis_FOUND)
  if(NOT TARGET Hiredis_lib)
    add_library(Hiredis_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Hiredis_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${Hiredis_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${Hiredis_LIBRARIES}")
endif(Hiredis_FOUND)