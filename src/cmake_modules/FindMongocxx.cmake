find_path(MONGOCXX_INCLUDE_DIRS
          NAMES mongocxx/client.hpp
          DOC "mongocxx include dir"
          PATH_SUFFIXES mongocxx/include/mongocxx/v_noabi)

find_library(MONGOCXX_LIBRARIES NAMES mongocxx DOC "mongocxx library" PATH_SUFFIXES mongocxx/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mongocxx
                                  DEFAULT_MSG
                                  MONGOCXX_INCLUDE_DIRS
                                  MONGOCXX_LIBRARIES)
mark_as_advanced(MONGOCXX_INCLUDE_DIRS MONGOCXX_LIBRARIES)

if(Mongocxx_FOUND)
  if (NOT TARGET Mongocxx_lib)
    add_library(Mongocxx_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Mongocxx_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${MONGOCXX_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${MONGOCXX_LIBRARIES}")

endif(Mongocxx_FOUND)

find_path(BSONCXX_INCLUDE_DIRS
          NAMES bsoncxx/json.hpp
          DOC "bsoncxx include dir"
          PATH_SUFFIXES mongocxx/include/bsoncxx/v_noabi)

find_library(BSONCXX_LIBRARIES NAMES bsoncxx DOC "bsoncxx library" PATH_SUFFIXES mongocxx/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Bsoncxx
                                  DEFAULT_MSG
                                  BSONCXX_INCLUDE_DIRS
                                  BSONCXX_LIBRARIES)
mark_as_advanced(BSONCXX_INCLUDE_DIRS BSONCXX_LIBRARIES)

if(Bsoncxx_FOUND)
  if (NOT TARGET Bsoncxx_lib)
    add_library(Bsoncxx_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Bsoncxx_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${BSONCXX_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${BSONCXX_LIBRARIES}")

endif(Bsoncxx_FOUND)