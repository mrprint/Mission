cmake_minimum_required (VERSION 2.8)
cmake_policy(SET CMP0054 OLD)

project (Mission)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(SFML_ROOT "${CMAKE_SOURCE_DIR}/thrdparty/SFML")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

include(CheckCXXCompilerFlag)

CHECK_CXX_COMPILER_FLAG("-std=c++98" COMPILER_SUPPORTS_CXX98)
CHECK_CXX_COMPILER_FLAG("-pedantic" COMPILER_SUPPORTS_PEDANTIC)
CHECK_CXX_COMPILER_FLAG("-fno-rtti" COMPILER_SUPPORTS_FNO_RTTI)
CHECK_CXX_COMPILER_FLAG("/GR-" COMPILER_SUPPORTS_GR_MINUS)

if(COMPILER_SUPPORTS_CXX98)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++98")
endif()

if(COMPILER_SUPPORTS_CXX98 AND COMPILER_SUPPORTS_PEDANTIC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
endif()

find_package(SFML 2.3 COMPONENTS system window graphics audio REQUIRED)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	find_package(SFML 2.3 COMPONENTS main REQUIRED)
endif()

include_directories(${SFML_INCLUDE_DIR})
add_executable(mission 
	src/main.cpp 
	src/world.cpp 
	src/engine.cpp
	src/spaces.cpp
)

if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC") AND COMPILER_SUPPORTS_GR_MINUS)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
elseif (COMPILER_SUPPORTS_FNO_RTTI)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set_property(TARGET mission PROPERTY WIN32_EXECUTABLE TRUE)
endif()

target_link_libraries(mission ${SFML_LIBRARIES})
