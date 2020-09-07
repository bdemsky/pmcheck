# A few common Makefile items

CC=../../clang
CXX=../../clang++

CXXFLAGS=-std=c++0x -pthread -Wall $(SANITIZE) -g -I../include

UNAME = $(shell uname)


