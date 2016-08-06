cmake_minimum_required (VERSION 2.8)
project (Mission)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(SFML_ROOT "${CMAKE_SOURCE_DIR}/thrdparty/SFML")
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -std=c++98")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++98")
endif()

find_package(SFML 2.3 COMPONENTS system window graphics audio REQUIRED)

include_directories(${SFML_INCLUDE_DIR})
add_executable(mission 
	src/main.cpp 
	src/world.cpp 
	src/engine.cpp
)
target_link_libraries(mission ${SFML_LIBRARIES})
