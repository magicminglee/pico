find_path(XLOG_INCLUDE_DIRS
          NAMES xlog/xlog.hpp
          DOC "xlog include dir"
          PATH_SUFFIXES xlog/include)

find_library(XLOG_LIBRARIES NAMES xlog DOC "xlog library" PATH_SUFFIXES xlog/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Xlog
                                  DEFAULT_MSG
                                  XLOG_INCLUDE_DIRS
                                  XLOG_LIBRARIES)
mark_as_advanced(XLOG_INCLUDE_DIRS XLOG_LIBRARIES)

if(Xlog_FOUND)
  if (NOT TARGET Xlog_lib)
    add_library(Xlog_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Xlog_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${XLOG_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${XLOG_LIBRARIES}")

endif(Xlog_FOUND)