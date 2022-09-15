find_path(LUAPB_INCLUDE_DIRS
  NAMES luapb/pb.h
  DOC "luapb include dir"
  PATH_SUFFIXES luapb/include)

find_library(LUAPB_LIBRARIES NAMES pb DOC "pb library" PATH_SUFFIXES luapb/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LuaPb
  DEFAULT_MSG
  LUAPB_INCLUDE_DIRS
  LUAPB_LIBRARIES)
mark_as_advanced(LUAPB_INCLUDE_DIRS LUAPB_LIBRARIES)

if(LuaPb_FOUND)
  if(NOT TARGET LuaPb_lib)
    add_library(LuaPb_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(LuaPb_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${LUAPB_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${LUAPB_LIBRARIES}")
endif(LuaPb_FOUND)