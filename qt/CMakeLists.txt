cmake_minimum_required(VERSION 3.1.0)

project(RaspberryPiQtApp)

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 11)

set(SOURCES
    qt_example.cpp
)

add_executable(RaspberryPiQtApp ${SOURCES})

target_link_libraries(RaspberryPiQtApp
    Qt5::Core
    Qt5::Widgets
    ${OpenCV_LIBS}
)