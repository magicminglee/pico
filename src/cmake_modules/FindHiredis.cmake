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
find_path(Hiredis_INCLUDE_DIRS
          NAMES hiredis/hiredis.h
          DOC "Hiredis include dir"
          PATH_SUFFIXES hiredis/include)

find_library(Hiredis_LIBRARIES NAMES hiredis DOC "Hiredis library" PATH_SUFFIXES hiredis/lib)

# handle the QUIETLY and REQUIRED arguments and set SASL2_FOUND to TRUE if all
# listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Hiredis
                                  DEFAULT_MSG
                                  Hiredis_INCLUDE_DIRS
                                  Hiredis_LIBRARIES)
mark_as_advanced(Hiredis_INCLUDE_DIRS Hiredis_LIBRARIES)

if(Hiredis_FOUND)
  if (NOT TARGET Hiredis_lib)
    add_library(Hiredis_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Hiredis_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${Hiredis_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${Hiredis_LIBRARIES}")

endif(Hiredis_FOUND)