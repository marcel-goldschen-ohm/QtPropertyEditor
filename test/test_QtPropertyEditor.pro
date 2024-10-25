TARGET = test_QtPropertyEditor
TEMPLATE = app
QT += core gui widgets

CONFIG += c++17

INCLUDEPATH += .
INCLUDEPATH += ..

HEADERS += ../QtPropertyEditor.h
SOURCES += ../QtPropertyEditor.cpp

HEADERS += test_QtPropertyEditor.h
SOURCES += test_QtPropertyEditor.cpp
