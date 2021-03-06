cmake_minimum_required(VERSION 2.8)
project(LibISR)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

#set(OPEN_NI_ROOT /home/carl/Work/3rdparty/OpenNI2/)

set(freenect2_INCLUDE_DIR /home/paler/freenect2/include/)
set(freenect2_LIBRARIES
  /home/paler/freenect2/lib/libfreenect2.so
  /home/paler/freenect2/lib/libfreenect2.so.0.2
  /home/paler/freenect2/lib/libfreenect2.so.0.2.0)

set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} $ENV{HOME}/freenect2/lib/cmake/freenect2)

set(Protobuf_LIBRARIES /usr/lib/x86_64-linux-gnu/libprotobuf.so.9)

find_package(GLUT REQUIRED)
find_package(OpenGL REQUIRED)
find_package(CUDA REQUIRED)
find_package(OpenNI REQUIRED)
find_package(OpenCV REQUIRED)

set(freenect2_DIR /home/freenect2/lib/cmake/freenect2)
add_definitions(-Dfreenect2_DIR)

set(OpenCV_INCLUDE_DIR /usr/local/include/opencv2)

include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${freenect2_INCLUDE_DIR})
include_directories(${OPENGL_INCLUDE_DIR})
include_directories(${GLUT_INCLUDE_DIR})
include_directories(${OpenNI_INCLUDE_DIR})
include_directories(${OpenCV_INCLUDE_DIR})

#set(OpenCV_LIBRARY /usr/local/lib/libopencv_core.so /usr/local/lib/libopencv_imgproc.so)
#link_directories(${OpenCV_LIBRARY_DIRS})

IF(MSVC_IDE)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-DUSING_CMAKE=1)
ELSE(MSVC_IDE)
  set(CFLAGS_WARN "-Wall -Wextra -Wno-unused-parameter -Wno-strict-aliasing")
  #set(CMAKE_CXX_FLAGS "-fPIC -O3 -march=native ${CFLAGS_WARN} ${CMAKE_CXX_FLAGS}  -std=c++11")
  set(CMAKE_CXX_FLAGS "-fPIC -g ${CFLAGS_WARN} ${CMAKE_CXX_FLAGS}  -std=c++11" )
ENDIF(MSVC_IDE)

if(APPLE)
  set(CUDA_HOST_COMPILER /usr/bin/clang)
endif(APPLE)

IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++ -Wno-deprecated-declarations -Wno-unused-function")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libstdc++")
ENDIF()


add_subdirectory(ORUtils)
add_subdirectory(LibISR)

set(MyProtobuf
	LibISR/Protobuf/result.pb.cc
	LibISR/Protobuf/result.pb.h
  LibISR/Protobuf/Event.h
  LibISR/Protobuf/Communication.h
  LibISR/Protobuf/Communication.cpp
)

add_executable(demo ${MyProtobuf} demo.cpp)
target_link_libraries(demo LibISR_ALL  ${Protobuf_LIBRARIES} ${freenect2_LIBRARIES} ${OpenCV_LIBS})


