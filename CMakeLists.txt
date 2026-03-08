cmake_minimum_required(VERSION 3.16)
project(matepaint VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

add_executable(matepaint
    src/matepaint.cpp
    src/matepaint.hpp
)

target_link_libraries(matepaint PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core)

install(TARGETS matepaint DESTINATION bin)
install(DIRECTORY data DESTINATION share/matepaint)
install(FILES org.kde.matepaint.desktop DESTINATION share/applications)
