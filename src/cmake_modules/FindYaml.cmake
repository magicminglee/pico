find_path(Yaml_INCLUDE_DIRS
          NAMES yaml-cpp/yaml.h
          DOC "Yaml include dir"
          PATH_SUFFIXES yaml/include)

find_library(Yaml_LIBRARIES
          NAMES yaml-cpp
          DOC "Yaml library"
          PATH_SUFFIXES yaml/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Yaml
                                  DEFAULT_MSG
                                  Yaml_INCLUDE_DIRS
                                  Yaml_LIBRARIES)
mark_as_advanced(Yaml_INCLUDE_DIRS Yaml_LIBRARIES)

if(Yaml_FOUND)
  if (NOT TARGET Yaml_lib)
    add_library(Yaml_lib INTERFACE IMPORTED)
  endif()
  set_target_properties(Yaml_lib
                        PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
                                   "${Yaml_INCLUDE_DIRS}"
                                   INTERFACE_LINK_LIBRARIES
                                   "${Yaml_LIBRARIES}")

endif(Yaml_FOUND)