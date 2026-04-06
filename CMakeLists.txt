cmake_minimum_required(VERSION 3.16)
project(matepaint VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
find_package(KF5 COMPONENTS I18n CoreAddons WidgetsAddons XmlGui REQUIRED)

add_executable(matepaint
    src/matepaint.cpp
    src/matepaint.hpp
    resources.qrc
)

target_link_libraries(matepaint PRIVATE Qt5::Widgets Qt5::Gui Qt5::Core KF5::I18n KF5::CoreAddons KF5::WidgetsAddons KF5::XmlGui)

install(TARGETS matepaint DESTINATION bin)
install(DIRECTORY data DESTINATION share/matepaint)
install(FILES org.kde.matepaint.desktop DESTINATION share/applications)
install(FILES matepaintui.rc DESTINATION share/kxmlgui5/matepaint)
