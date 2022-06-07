# Find sasl2
#
# Find the sasl2 includes and library
#
# if you nee to add a custom library search path, do it via via
# CMAKE_PREFIX_PATH
#
# This module defines SASL2_INCLUDE_DIRS, where to find header, etc.
# SASL2_LIBRARIES, the libraries needed to use sasl2. SASL2_FOUND, If
# false, do not try to use sasl2. 
# Sasl2_lib - The imported target library.

# only look in default directories
find_path(fmt_INCLUDE_DIRS
          NAMES fmt/format.h
          DOC "fmt include dir"
          PATH_SUFFIXES fmt/include)

find_library(fmt_LIBRARIES NAMES fmt DOC "fmt library" PATH_SUFFIXES fmt/lib)

# handle the QUIETLY and REQUIRED arguments and set SASL2_FOUND to TRUE if all
# listed variables are TRUE, hide their existence from configuration view
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