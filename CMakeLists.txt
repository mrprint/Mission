cmake_minimum_required (VERSION 2.8)
project (Mission)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(SFML_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/thrdparty/SFML")
endif()

find_package(SFML 2.3 COMPONENTS system window graphics audio REQUIRED)

include_directories(${SFML_INCLUDE_DIR})
add_executable(mission 
	src/main.cpp 
	src/world.cpp 
	src/engine.cpp
)
target_link_libraries(mission ${SFML_LIBRARIES})
