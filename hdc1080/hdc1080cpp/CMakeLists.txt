cmake_minimum_required(VERSION 2.8)

set(PROJECT_NAME hdc1080)

project(${PROJECT_NAME})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../src)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/../src)
include_directories(include)

AUX_SOURCE_DIRECTORY(/home/pi4/hardware/mdd/hdc1080/src DIR_SRCS)
add_executable( ${PROJECT_NAME} ${DIR_SRCS})

target_link_libraries(${PROJECT_NAME} wiringPi)
# find_library(wiringPi_LIB wiringPi)
# target_link_libraries(${PROJECT_NAME} ${wiringPi_LIB})

# message("OpenCV_LIBS = ${OpenCV_INCLUDE_DIRS}")
# target_link_libraries(${PROJECT_NAME} "/home/pi4/socket.io-client-cpp/build/libsioclient.a")
# target_link_libraries( ${OpenCV_LIBS})
# target_link_libraries(${PROJECT_NAME} ${CURL_LIBS})
# target_link_libraries(${PROJECT_NAME} curl)
# target_link_libraries(${PROJECT_NAME} ncnn)
