# Created by Marcel Paz Goldschen-Ohm <marcel.goldschen@gmail.com>

# !!! Set env variable QT6_DIR to <Qt install prefix>/lib/cmake/Qt6

cmake_minimum_required(VERSION 3.11)

set(PROJECT_NAME test_QtPropertyEditor)
project(${PROJECT_NAME} LANGUAGES CXX)

include(FetchContent)

# Fetch QtPropertyEditor repository from GitHub.
set(REPO QtPropertyEditor)
string(TOLOWER ${REPO} REPOlc)
FetchContent_Declare(${REPO}
  GIT_REPOSITORY "https://github.com/marcel-goldschen-ohm/QtPropertyEditor.git"
)
FetchContent_GetProperties(${REPO})
if(NOT ${REPOlc}_POPULATED)
  FetchContent_Populate(${REPO})
  message(STATUS "${REPO} source dir: ${${REPOlc}_SOURCE_DIR}")
  message(STATUS "${REPO} binary dir: ${${REPOlc}_BINARY_DIR}")
endif()

set(CMAKE_CXX_STANDARD 17) # This is equal to QMAKE_CXX_FLAGS += -std=c++0x
set(CMAKE_INCLUDE_CURRENT_DIR ON) # Search in current dir.
set(CMAKE_AUTOMOC ON) # Run moc automatically for Qt.
set(CMAKE_AUTOUIC ON) # Run uic automatically for *.ui files.
set(CMAKE_AUTORCC ON) # Run automatically for *.qrc files.

if(APPLE AND EXISTS /usr/local/opt/qt6)
  # Homebrew installs Qt6, ensure it can be found by CMake.
  list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt6")
endif()

# Find required packages.
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# Build fetched repository as a static library.
add_library(QtPropertyEditor STATIC ${qtpropertyeditor_SOURCE_DIR}/QtPropertyEditor.cpp ${qtpropertyeditor_SOURCE_DIR}/QtPropertyEditor.h)
target_link_libraries(QtPropertyEditor Qt6::Widgets)

# Build project executable.
add_executable(${PROJECT_NAME} test_QtPropertyEditor.cpp test_QtPropertyEditor.h)
target_include_directories(${PROJECT_NAME} PUBLIC ${qtpropertyeditor_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} Qt6::Widgets QtPropertyEditor)

