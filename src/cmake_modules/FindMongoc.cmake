find_path(MONGOC_INCLUDE_DIRS
          NAMES mongoc.h
          DOC "mongoc include dir"
          PATH_SUFFIXES mongoc/include/libmongoc-1.0)

find_library(MONGOC_LIBRARIES NAMES mongoc-1.0 DOC "mongoc library" PATH_SUFFIXES mongoc/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mongoc
                                  DEFAULT_MSG
                                  MONGOC_INCLUDE_DIRS
                                  MONGOC_LIBRARIES)
mark_as_advanced(MONGOC_INCLUDE_DIRS MONGOC_LIBRARIES)

if(Mongoc_FOUND)
  if (NOT TARGET Mongoc_lib)
    add_library(Mongoc_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Mongoc_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${MONGOC_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${MONGOC_LIBRARIES}")

endif(Mongoc_FOUND)