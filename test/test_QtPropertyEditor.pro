TARGET = test_QtPropertyEditor
TEMPLATE = app
QT += core gui widgets
CONFIG += c++17

OBJECTS_DIR = Debug/.obj
MOC_DIR = Debug/.moc
RCC_DIR = Debug/.rcc
UI_DIR = Debug/.ui

DEFINES += DEBUG

INCLUDEPATH += ..

HEADERS += ../QtPropertyEditor.h
SOURCES += ../QtPropertyEditor.cpp

HEADERS += test_QtPropertyEditor.h
SOURCES += test_QtPropertyEditor.cpp

# Upgrade for Qt 6 compatibility
QT += core gui widgets

# Enforce higher C++ standard for modern Qt
CONFIG += c++17

# Ensure that relative paths work consistently in different build systems
CONFIG += qtquickcompiler

# Modern build directories (optional change)
OBJECTS_DIR = build/debug/.obj
MOC_DIR = build/debug/.moc
RCC_DIR = build/debug/.rcc
UI_DIR = build/debug/.ui
