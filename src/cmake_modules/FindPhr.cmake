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
find_path(PHR_INCLUDE_DIRS
          NAMES picohttpparser/picohttpparser.h
          DOC "phr include dir"
          PATH_SUFFIXES phr/include)

find_library(PHR_LIBRARIES NAMES phr DOC "phr library" PATH_SUFFIXES phr/lib)

# handle the QUIETLY and REQUIRED arguments and set SASL2_FOUND to TRUE if all
# listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Phr
                                  DEFAULT_MSG
                                  PHR_INCLUDE_DIRS
                                  PHR_LIBRARIES)
mark_as_advanced(PHR_INCLUDE_DIRS PHR_LIBRARIES)

if(Phr_FOUND)
  if (NOT TARGET Phr_lib)
    add_library(Phr_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Phr_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${PHR_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${PHR_LIBRARIES}")

endif(Phr_FOUND)