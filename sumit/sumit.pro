TEMPLATE = app
TARGET = sumit
DESTDIR = ../bin

QT += core
QT -= gui
CONFIG -= app_bundle #Please apple, don't make a bundle
CONFIG += c++11
OBJECTS_DIR = ../build
MOC_DIR=../build

SOURCES += sumitmain.cpp \
    clparams.cpp
HEADERS += sumit.h \
    clparams.h
SOURCES += sumit.cpp
