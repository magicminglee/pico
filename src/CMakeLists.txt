cmake_minimum_required(VERSION 3.10..3.24)

project(PICO)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
add_definitions(-DMAJOR=${VERSION_MAJOR})
add_definitions(-DMINOR=${VERSION_MINOR})

option(TESTSOPT "This is a default option for TESTS" OFF)
message(STATUS "compiler: " ${CMAKE_CXX_COMPILER_ID})

include(CheckIncludeFileCXX)

# set(CMAKE_CXX_STANDARD 20)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules/)

check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)

if(NOT "${CMAKE_CXX_STANDARD}" STREQUAL "")
# Do nothing
elseif(HAS_ANY AND HAS_STRING_VIEW AND HAS_COROUTINE)
    set(CMAKE_CXX_STANDARD 20)
elseif(HAS_ANY AND HAS_STRING_VIEW)
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 14)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} main.cpp)

list(APPEND ALL_BIN_TARGETS ${PROJECT_NAME})

if("${TESTSOPT}" STREQUAL "ON")
    add_executable(test)
    find_package(Catch2 REQUIRED)
    target_link_libraries(test PUBLIC Catch2_lib)
    list(APPEND ALL_BIN_TARGETS test)
endif()

# ############################################
function(alltargets_link libname)
    foreach(name ${ALL_BIN_TARGETS})
        target_link_libraries(${name} PUBLIC ${libname})
    endforeach()
endfunction()

# ###########################################
find_package(Threads REQUIRED)
alltargets_link(Threads::Threads)
find_package(Fmt REQUIRED)
alltargets_link(Fmt_lib)
find_package(Sasl2 REQUIRED)
alltargets_link(Sasl2_lib)
find_package(Mongoc REQUIRED)
alltargets_link(Mongoc_lib)
find_package(Bson REQUIRED)
alltargets_link(Bson_lib)
find_package(Mongocxx REQUIRED)
alltargets_link(Mongocxx_lib)
find_package(Bsoncxx REQUIRED)
alltargets_link(Bsoncxx_lib)
find_package(Lua REQUIRED)
alltargets_link(Lua_lib)
find_package(LuaPb REQUIRED)
alltargets_link(LuaPb_lib)
find_package(Json REQUIRED)
alltargets_link(Json_lib)
find_package(Libevent REQUIRED)
alltargets_link(Libevent_lib)
find_package(Openssl REQUIRED)
alltargets_link(Openssl_lib)
find_package(Hiredis REQUIRED)
alltargets_link(Hiredis_lib)
find_package(Redis++ REQUIRED)
alltargets_link(Redis++_lib)
find_package(Cxxopt REQUIRED)
alltargets_link(Cxxopt_lib)
find_package(Yaml REQUIRED)
alltargets_link(Yaml_lib)
find_package(Jwtcxx REQUIRED)
alltargets_link(Jwtcxx_lib)
find_package(Nghttp2 REQUIRED)
alltargets_link(Nghttp2_lib)
find_package(LLhttp REQUIRED)
alltargets_link(LLhttp_lib)
find_package(Clickhouse REQUIRED)
alltargets_link(Clickhouse_lib)
find_package(Spdlog REQUIRED)
alltargets_link(Spdlog_lib)
find_package(Protobuf REQUIRED)
alltargets_link(Protobuf_lib)

# ###########################################
if(CMAKE_CXX_STANDARD LESS 17)
    # With C++14, use boost to support any, string_view and filesystem
    message(STATUS "use c++14")
    find_package(Boost 1.61.0 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Boost::boost)
elseif(CMAKE_CXX_STANDARD LESS 20)
    message(STATUS "use c++17")
else()
    message(STATUS "use c++20")
endif()

if(NOT("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Release"))
    message(FATAL_ERROR "FATAL: no build type supplyed are not allowed. You should add -DCMAKE_BUILD_TYPE=Debug|Release option. ")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    add_definitions(-D_DEBUG_MODE)
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    add_definitions(-D_RELEASE_MODE)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -Wall -Wno-unused-function -Wno-unused-variable")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -Wall ")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# or use MATCHES to see if actual system name
# Darwin is Apple's system name
if(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
    message("This is a ${CMAKE_SYSTEM_NAME} system")
    add_definitions(-DPLATFORMOS="Darwin")
    add_definitions(-DDARWIN_PLATFORMOS)
elseif(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    message("This is a Windows System")
    add_definitions(-DPLATFORMOS="Windows")
    add_definitions(-DWINDOWS_PLATFORMOS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
elseif(${CMAKE_SYSTEM_NAME} MATCHES Linux)
    message("This is a Linux System")
    add_definitions(-DPLATFORMOS="Linux")
    add_definitions(-DLINUX_PLATFORMOS)
    alltargets_link(stdc++fs)
endif()

# Disable in-source builds to prevent source tree corruption.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
    message(FATAL_ERROR "FATAL: In-source builds are not allowed. You should create a separate directory for build files. ")
endif()

aux_source_directory(framework FRAMEWORK_SRC)
aux_source_directory(gamelibs GAMELIBS_SRC)
aux_source_directory(proto PROTO_SRC)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(${PROJECT_NAME}
    PRIVATE
    ${SRC_DIR}
    ${FRAMEWORK_SRC}
    ${GAMELIBS_SRC}
    ${PROTO_SRC}
)

if("${TESTSOPT}" STREQUAL "ON")
    aux_source_directory(tests TESTS_SRC)
    target_include_directories(test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_sources(test
        PRIVATE
        ${TESTS_SRC}
        ${FRAMEWORK_SRC}
        ${GAMELIBS_SRC}
        ${PROTO_SRC}
    )
endif()