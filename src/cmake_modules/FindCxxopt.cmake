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
find_path(Cxxopt_INCLUDE_DIRS
          NAMES cxxopts.hpp
          DOC "Cxxopt include dir"
          PATH_SUFFIXES cxxopt/include)

# handle the QUIETLY and REQUIRED arguments and set SASL2_FOUND to TRUE if all
# listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cxxopt
                                  DEFAULT_MSG
                                  Cxxopt_INCLUDE_DIRS)
                                  
mark_as_advanced(Cxxopt_INCLUDE_DIRS)

if(Cxxopt_FOUND)
  if (NOT TARGET Cxxopt_lib)
    add_library(Cxxopt_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Cxxopt_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${Cxxopt_INCLUDE_DIRS}")

endif(Cxxopt_FOUND)