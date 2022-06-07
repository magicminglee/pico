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
find_path(OPENSSL_INCLUDE_DIRS
          NAMES openssl/ssl.h
          DOC "openssl include dir"
          PATH_SUFFIXES openssl/include)

find_library(crypto_LIBRARIES
          NAMES crypto
          DOC "crypto library"
          PATH_SUFFIXES openssl/lib)

find_library(ssl_LIBRARIES
          NAMES ssl
          DOC "ssl library"
          PATH_SUFFIXES openssl/lib)

# handle the QUIETLY and REQUIRED arguments and set SASL2_FOUND to TRUE if all
# listed variables are TRUE, hide their existence from configuration view
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Openssl
                                  DEFAULT_MSG
                                  OPENSSL_INCLUDE_DIRS
                                  crypto_LIBRARIES
                                  ssl_LIBRARIES)
mark_as_advanced(OPENSSL_INCLUDE_DIRS OPENSSL_LIBRARIES)

if(Openssl_FOUND)
  if (NOT TARGET Openssl_lib)
    add_library(Openssl_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Openssl_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${OPENSSL_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${crypto_LIBRARIES};${ssl_LIBRARIES}"
                                   )

endif(Openssl_FOUND)