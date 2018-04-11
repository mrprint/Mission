cmake_minimum_required (VERSION 2.8)
cmake_policy(SET CMP0054 OLD)

project (Mission)

option(NO_THREADS "Single-threaded, synchronous mode" OFF)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules")

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(SFML_ROOT "${CMAKE_SOURCE_DIR}/thrdparty/SFML")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(SRCS ${SRCS}
    src/coworker.hpp
    src/engine.hpp
    src/hfstorage.hpp
    src/pathfinding.hpp
    src/settings.hpp
    src/spaces.hpp
    src/world.hpp
    src/mathapp.hpp
    src/main.cpp
    src/world.cpp
    src/engine.cpp
    src/assets.cpp
    src/spaces.cpp)

if(NO_THREADS)
    add_definitions(-DNO_THREADS)
    set(SRCS ${SRCS} src/coworker_sync.hpp)
    set(SRCS ${SRCS} src/coworker_sync.cpp)
else()
    add_definitions(-DWITH_THREADS)
    set(SRCS ${SRCS} src/coworker_async.hpp)
    set(SRCS ${SRCS} src/coworker_async.cpp)
endif()

include(CheckCXXCompilerFlag)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
CHECK_CXX_COMPILER_FLAG("/std:c++17" COMPILER_SUPPORTS_W_CXX17)
CHECK_CXX_COMPILER_FLAG("/GR-" COMPILER_SUPPORTS_W_GRMINUS)
else()
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
CHECK_CXX_COMPILER_FLAG("-std=c++1z" COMPILER_SUPPORTS_CXX1z)
CHECK_CXX_COMPILER_FLAG("-pedantic" COMPILER_SUPPORTS_PEDANTIC)
CHECK_CXX_COMPILER_FLAG("-fno-rtti" COMPILER_SUPPORTS_FNO_RTTI)
endif()

if(COMPILER_SUPPORTS_W_CXX17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
elseif(COMPILER_SUPPORTS_CXX17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
elseif(COMPILER_SUPPORTS_CXX1z)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
endif()

if((COMPILER_SUPPORTS_CXX17
    OR COMPILER_SUPPORTS_CXX1z)
    AND COMPILER_SUPPORTS_PEDANTIC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic")
endif()

find_package(SFML 2.3 COMPONENTS system window graphics audio REQUIRED)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    find_package(SFML 2.3 COMPONENTS main REQUIRED)
endif()

find_package(Threads REQUIRED)

include_directories(${SFML_INCLUDE_DIR})
add_executable(${PROJECT_NAME} ${SRCS})

if (COMPILER_SUPPORTS_W_GRMINUS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR-")
elseif (COMPILER_SUPPORTS_FNO_RTTI)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")
endif()

if((NOT NO_THREADS) AND THREADS_HAVE_PTHREAD_ARG)
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY COMPILE_OPTIONS "-pthread")
    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE TRUE)
endif()

if(CMAKE_THREAD_LIBS_INIT)
    target_link_libraries(${PROJECT_NAME} "${CMAKE_THREAD_LIBS_INIT}")
endif()

target_link_libraries(${PROJECT_NAME} ${SFML_LIBRARIES})

################################################################################
# Copyright(c) 2017 https://github.com/mrprint
#
# Permission is hereby granted, free of charge, to any person obtaining a copy 
# of this software and associated documentation files(the "Software"), to deal 
# in the Software without restriction, including without limitation the rights 
# to use, copy, modify, merge, publish, distribute, sublicense, and / or sell 
# copies of the Software, and to permit persons to whom the Software is 
# furnished to do so, subject to the following conditions :
#
# The above copyright notice and this permission notice shall be included in 
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
# SOFTWARE.
