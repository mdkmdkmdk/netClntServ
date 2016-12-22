TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp

QMAKE_CXXFLAGS += -std=gnu++0x -pthread -lpthread
LIBS += -L/usr/include -pthread
