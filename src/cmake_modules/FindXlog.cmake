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
find_path(XLOG_INCLUDE_DIRS
          NAMES xlog/xlog.hpp
          DOC "xlog include dir"
          PATH_SUFFIXES xlog/include)

find_library(XLOG_LIBRARIES NAMES xlog DOC "xlog library" PATH_SUFFIXES xlog/lib)

# handle the QUIETLY and REQUIRED arguments and set SASL2_FOUND to TRUE if all
# listed variables are TRUE, hide their existence from configuration view
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