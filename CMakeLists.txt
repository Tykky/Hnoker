cmake_minimum_required(VERSION 3.2)

project(Hnoker
	VERSION 0.0.1
	DESCRIPTION "hnoker XD"
	LANGUAGES CXX
)

find_package(Boost 1.74.0 REQUIRED COMPONENTS system)

set(srcs
	src/main.cpp
)

add_executable(hnoker ${srcs})
target_link_libraries(hnoker PRIVATE ${Boost_LIBRARIES})

set_property(TARGET hnoker PROPERTY CXX_STANDARD 20)