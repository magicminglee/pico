find_path(Spdlog_INCLUDE_DIRS
  NAMES spdlog/spdlog.h
  DOC "Spdlog include dir"
  PATH_SUFFIXES spdlog/include)

find_library(Spdlog_LIBRARIES NAMES spdlog DOC "Spdlog library" PATH_SUFFIXES spdlog/lib)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Spdlog
  DEFAULT_MSG
  Spdlog_INCLUDE_DIRS
  Spdlog_LIBRARIES)

mark_as_advanced(Spdlog_INCLUDE_DIRS Spdlog_LIBRARIES)

if(Spdlog_FOUND)
  if(NOT TARGET Spdlog_lib)
    add_library(Spdlog_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Spdlog_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${Spdlog_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${Spdlog_LIBRARIES}")
endif(Spdlog_FOUND)