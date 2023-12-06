cmake_minimum_required(VERSION 3.2)

project(Hnoker
	VERSION 0.0.1
	DESCRIPTION "hnoker XD"
	LANGUAGES CXX
)

if (DEFINED CACHE{USING_VCPKG})
	execute_process(COMMAND vcpkg install)
endif()

find_package(Boost 1.83.0 REQUIRED COMPONENTS system)
find_package(spdlog 1.12.0 REQUIRED)

set(srcs
	src/main.cpp
	src/networking.hpp
    src/networking.cpp
	src/logging.hpp
    src/logging.cpp
	src/player.hpp
	src/player.cpp
)

add_executable(hnoker ${srcs})
target_include_directories(hnoker PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(hnoker PRIVATE ${Boost_LIBRARIES} spdlog::spdlog)

set_property(TARGET hnoker PROPERTY CXX_STANDARD 20)
