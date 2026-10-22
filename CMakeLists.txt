cmake_minimum_required(VERSION 3.21)

project(Pyke CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-march=native)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_options(-g -O0)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	add_compile_options(-Ofast -funroll-loops)
else()
	add_compile_options(-O3 -funroll-loops)
endif()

add_executable(pyke main.cpp board.cpp)

add_executable(pyke_test EXCLUDE_FROM_ALL test.cpp board.cpp)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_compile_options(pyke_test PRIVATE -Wno-missing-profile)
endif()
add_custom_target(check COMMAND pyke_test DEPENDS pyke_test)

if(CMAKE_BUILD_TYPE STREQUAL "Generate")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-generate")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-use")
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
