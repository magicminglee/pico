find_path(PHR_INCLUDE_DIRS
          NAMES picohttpparser/picohttpparser.h
          DOC "phr include dir"
          PATH_SUFFIXES phr/include)

find_library(PHR_LIBRARIES NAMES phr DOC "phr library" PATH_SUFFIXES phr/lib)

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