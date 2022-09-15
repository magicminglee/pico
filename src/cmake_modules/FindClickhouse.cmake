find_path(Clickhouse_INCLUDE_DIRS
  NAMES clickhouse/client.h
  DOC "clickhouse include dir"
  PATH_SUFFIXES clickhousecxx/include)

find_path(Clickhousecontrib_INCLUDE_DIRS
  NAMES absl/numeric/int128.h
  DOC "clickhouse include dir"
  PATH_SUFFIXES clickhousecxx/contrib)

find_library(Clickhouse_LIBRARIES NAMES clickhouse-cpp-lib DOC "Clickhouse library" PATH_SUFFIXES clickhousecxx/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Clickhouse
  DEFAULT_MSG
  Clickhouse_INCLUDE_DIRS
  Clickhousecontrib_INCLUDE_DIRS
  Clickhouse_LIBRARIES)

mark_as_advanced(Clickhouse_INCLUDE_DIRS Clickhousecontrib_INCLUDE_DIRS Clickhouse_LIBRARIES)

if(Clickhouse_FOUND)
  if(NOT TARGET Clickhouse_lib)
    add_library(Clickhouse_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Clickhouse_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${Clickhouse_INCLUDE_DIRS};${Clickhousecontrib_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${Clickhouse_LIBRARIES}")
endif(Clickhouse_FOUND)