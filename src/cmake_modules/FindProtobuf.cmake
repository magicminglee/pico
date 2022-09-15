find_path(Protobuf_INCLUDE_DIRS
  NAMES google/protobuf/message.h
  DOC "Protobuf include dir"
  PATH_SUFFIXES protobuf/include)

find_library(Protobuf_LIBRARIES NAMES protobuf DOC "Protobuf library" PATH_SUFFIXES protobuf/lib)
find_library(ProtobufLite_LIBRARIES NAMES protobuf-lite DOC "Protobuf library" PATH_SUFFIXES protobuf/lib)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Protobuf
  DEFAULT_MSG
  Protobuf_INCLUDE_DIRS
  Protobuf_LIBRARIES
  ProtobufLite_LIBRARIES
)

mark_as_advanced(Protobuf_INCLUDE_DIRS Protobuf_LIBRARIES ProtobufLite_LIBRARIES)

if(Protobuf_FOUND)
  if(NOT TARGET Protobuf_lib)
    add_library(Protobuf_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Protobuf_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${Protobuf_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${Protobuf_LIBRARIES};${ProtobufLite_LIBRARIES}")
endif(Protobuf_FOUND)