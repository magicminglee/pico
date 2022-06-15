cmake_minimum_required(VERSION 3.10.0)

project(PICO)

message(STATUS "compiler: " ${CMAKE_CXX_COMPILER_ID})

include(CheckIncludeFileCXX)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake_modules/)

check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)
if (NOT "${CMAKE_CXX_STANDARD}" STREQUAL "")
    # Do nothing
elseif (HAS_ANY AND HAS_STRING_VIEW AND HAS_COROUTINE)
    set(CMAKE_CXX_STANDARD 20)
elseif (HAS_ANY AND HAS_STRING_VIEW)
    set(CMAKE_CXX_STANDARD 17)
else ()
    set(CMAKE_CXX_STANDARD 14)
endif ()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} main.cpp)

############################################
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
find_package(Fmt REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Fmt_lib)
find_package(Sasl2 REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Sasl2_lib)
find_package(Mongoc REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Mongoc_lib)
find_package(Bson REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Bson_lib)
find_package(Mongocxx REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Mongocxx_lib)
find_package(Bsoncxx REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Bsoncxx_lib)
find_package(Phr REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Phr_lib)
find_package(Json REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Json_lib)
find_package(Libevent REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Libevent_lib)
find_package(Openssl REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Openssl_lib)
find_package(Hiredis REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Hiredis_lib)
find_package(Redis++ REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Redis++_lib)
find_package(Cxxopt REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Cxxopt_lib)
find_package(Yaml REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Yaml_lib)
find_package(Jwtcxx REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Jwtcxx_lib)
############################################

if (CMAKE_CXX_STANDARD LESS 17)
    # With C++14, use boost to support any, string_view and filesystem
    message(STATUS "use c++14")
    find_package(Boost 1.61.0 REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Boost::boost)
elseif (CMAKE_CXX_STANDARD LESS 20)
    message(STATUS "use c++17")
else ()
    message(STATUS "use c++20")
endif ()

if(NOT ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR "${CMAKE_BUILD_TYPE}" STREQUAL "Release"))
	message(FATAL_ERROR "FATAL: no build type supplyed are not allowed. You should add -DCMAKE_BUILD_TYPE=Debug|Release option. ")
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
    target_link_libraries(${PROJECT_NAME} PUBLIC stdc++fs)
endif()

# Disable in-source builds to prevent source tree corruption.
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "FATAL: In-source builds are not allowed. You should create a separate directory for build files. ")
endif()

aux_source_directory(framework FRAMEWORK_SRC)
aux_source_directory(gamelibs GAMELIBS_SRC)


target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_sources(${PROJECT_NAME}
               PRIVATE
               ${SRC_DIR}
               ${FRAMEWORK_SRC}
               ${GAMELIBS_SRC})

