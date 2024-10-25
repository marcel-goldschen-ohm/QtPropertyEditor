# Created by Marcel Paz Goldschen-Ohm <marcel.goldschen@gmail.com>

cmake_minimum_required(VERSION 3.21) # Qt6

set(PROJECT_NAME test_QtPropertyEditor)
project(${PROJECT_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON) # Search in current dir.
set(CMAKE_AUTOMOC ON) # Run moc automatically for Qt.
set(CMAKE_AUTOUIC ON) # Run uic automatically for *.ui files.
set(CMAKE_AUTORCC ON) # Run automatically for *.qrc files.

if(APPLE AND EXISTS /usr/local/opt/qt6)
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt6")
endif()

find_package(Qt6 COMPONENTS Widgets REQUIRED) # Qt6

# Build project executable.
add_executable(${PROJECT_NAME}
    ../QtPropertyEditor.h
    ../QtPropertyEditor.cpp
    test_QtPropertyEditor.h
    test_QtPropertyEditor.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ../
)

target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets) # Qt6
