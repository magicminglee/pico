find_path(fmt_INCLUDE_DIRS
          NAMES fmt/format.h
          DOC "fmt include dir"
          PATH_SUFFIXES fmt/include)

find_library(fmt_LIBRARIES NAMES fmt DOC "fmt library" PATH_SUFFIXES fmt/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Fmt
                                  DEFAULT_MSG
                                  fmt_INCLUDE_DIRS
                                  fmt_LIBRARIES)

mark_as_advanced(fmt_INCLUDE_DIRS fmt_LIBRARIES)

if(Fmt_FOUND)
  if (NOT TARGET Fmt_lib)
    add_library(Fmt_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Fmt_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${fmt_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${fmt_LIBRARIES}")

endif(Fmt_FOUND)