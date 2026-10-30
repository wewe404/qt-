QT += core gui widgets

CONFIG += c++17 warn_on

TARGET = Calculator
TEMPLATE = app

SOURCES += \
    main.cpp \
    calculator.cpp

HEADERS += \
    calculator.h

FORMS += \
    calculator.ui

# MinGW 下中文源码按 UTF-8 编译
win32-g++: QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
