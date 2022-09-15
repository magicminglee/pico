find_path(LUA_INCLUDE_DIRS
  NAMES lua.hpp
  DOC "lua include dir"
  PATH_SUFFIXES lua/include)

find_library(LUA_LIBRARIES NAMES lua DOC "lua library" PATH_SUFFIXES lua/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Lua
  DEFAULT_MSG
  LUA_INCLUDE_DIRS
  LUA_LIBRARIES
)
mark_as_advanced(LUA_INCLUDE_DIRS LUA_LIBRARIES)

if(Lua_FOUND)
  if(NOT TARGET Lua_lib)
    add_library(Lua_lib INTERFACE IMPORTED)
  endif()

  set_target_properties(Lua_lib
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES
    "${LUA_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES
    "${LUA_LIBRARIES}")
endif(Lua_FOUND)