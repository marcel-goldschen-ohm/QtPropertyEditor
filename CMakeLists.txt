# Created by Marcel Paz Goldschen-Ohm <marcel.goldschen@gmail.com>

# !!! Set env variable QT5_DIR to <Qt install prefix>/lib/cmake/Qt5
# For macx xcode project: cmake -G Xcode <dir of CMakeLists.txt>

# cmake_minimum_required(VERSION 3.11)
cmake_minimum_required(VERSION 3.21)

set(PROJECT_NAME QtPropertyEditor)
project(${PROJECT_NAME} LANGUAGES CXX)

## Qt5 # set(CMAKE_CXX_STANDARD 11) # This is equal to QMAKE_CXX_FLAGS += -std=c++0x
## Qt6
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON) # Search in current dir.
set(CMAKE_AUTOMOC ON) # Run moc automatically for Qt.
set(CMAKE_AUTOUIC ON) # Run uic automatically for *.ui files.
set(CMAKE_AUTORCC ON) # Run automatically for *.qrc files.

if(APPLE AND EXISTS /usr/local/opt/qt5)
  # Homebrew installs Qt5 (up to at least 5.9.1) in
  # /usr/local/qt5, ensure it can be found by CMake since
  # it is not in the default /usr/local prefix.
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt5")
endif()

# Find required packages.
## Qt5 # find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt6 COMPONENTS Widgets REQUIRED) # Qt6

# Build project as a static library.
add_library(${PROJECT_NAME} STATIC QtPropertyEditor.cpp QtPropertyEditor.h)

## Qt5 # qt5_use_modules(${PROJECT_NAME} Widgets)
target_link_libraries(${PROJECT_NAME} Qt6::Widgets)

target_link_libraries(${PROJECT_NAME} ${QT_LIBRARIES})


