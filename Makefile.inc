CXXFLAGS += -Wall -pthread $(INCLUDE_DIRS)
CFLAGS += -Wall -pthread $(INCLUDE_DIRS)
INCLUDE_DIRS=-I./include -I../include

CC=gcc
CXX=g++

#CXXFLAGS+=-O0 -fprofile-arcs -ftest-coverage
#CXXFLAGS+=-O0 --coverage

# change to reflect path to the CppUnit-Framework
CPPUNIT_PATH=/usr/include/cppunit

SLIBC_NAME=slibc

SLIBC_LIB=lib$(SLIBC_NAME).a

SLIBC_LIB_SH=lib$(SLIBC_NAME).so

SLIBC_LIB_SO=lib$(SLIBC_NAME).so

SLIBC_LIB_SONAME=lib$(SLIBC_NAME).so.$(VER_MAJOR).$(VER_MINOR).$(VER_RELEASE)

libdir = /usr/lib
includedir = /usr/include/
mandir = /usr/share/man/
