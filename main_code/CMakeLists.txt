cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME main_code)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 11)
set(ncnn_DIR "/home/pi4/ncnn/build/install/lib/cmake/ncnn" CACHE PATH "Directory that contains ncnnConfig.cmake")

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(OpenCV REQUIRED)

find_package(ncnn REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src)
include_directories(include)

AUX_SOURCE_DIRECTORY(./src DIR_SRCS)
add_executable( ${PROJECT_NAME} ${DIR_SRCS})

target_link_libraries(${PROJECT_NAME} wiringPi bcm2835 curl pthread ncnn)

target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Widgets ${OpenCV_LIBS})
# find_library(wiringPi_LIB wiringPi)
# target_link_libraries(${PROJECT_NAME} ${wiringPi_LIB})

# message("OpenCV_LIBS = ${OpenCV_INCLUDE_DIRS}")
# target_link_libraries(${PROJECT_NAME} "/home/pi4/socket.io-client-cpp/build/libsioclient.a")
# target_link_libraries( ${OpenCV_LIBS})
# target_link_libraries(${PROJECT_NAME} ${CURL_LIBS})
# target_link_libraries(${PROJECT_NAME} curl)
# target_link_libraries(${PROJECT_NAME} ncnn)
