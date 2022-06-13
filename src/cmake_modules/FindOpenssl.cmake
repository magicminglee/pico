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