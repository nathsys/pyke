cmake_minimum_required(VERSION 3.21)

project(Pyke CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Ofast -funroll-loops -march=native)

add_executable(pyke main.cpp board.cpp)

if(CMAKE_BUILD_TYPE STREQUAL "Generate")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-generate")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-use")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
endif()

add_custom_target(generate
	COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Generate ${CMAKE_SOURCE_DIR} && make
	COMMENT "Building in Generate mode"
)

add_custom_target(release
	COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release ${CMAKE_SOURCE_DIR} && make
	COMMENT "Building in Release mode"
)

add_custom_target(debug
	COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Debug ${CMAKE_SOURCE_DIR} && make
	COMMENT "Building in Debug mode"
)

add_dependencies(generate pyke)
add_custom_command(TARGET generate POST_BUILD
	COMMAND ./pyke 6
	COMMAND ./pyke 7
	COMMENT "Generating profile data for profile-guided optimization"
)

add_dependencies(release generate)
