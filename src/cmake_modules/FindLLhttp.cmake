find_path(LLhttp_INCLUDE_DIRS
  NAMES llhttp.h
  DOC "llhttp include dir"
  PATH_SUFFIXES llhttp/include)

find_library(LLhttp_LIBRARIES NAMES llhttp DOC "LLhttp library" PATH_SUFFIXES llhttp/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LLhttp
  DEFAULT_MSG
  LLhttp_INCLUDE_DIRS
  LLhttp_LIBRARIES)

mark_as_advanced(LLhttp_INCLUDE_DIRS LLhttp_LIBRARIES)

if(LLhttp_FOUND)
  if(NOT TARGET LLhttp_lib)
    add_library(LLhttp_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(LLhttp_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${LLhttp_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${LLhttp_LIBRARIES}")
endif(LLhttp_FOUND)