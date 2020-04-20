all: build/bin/runtests

SOURCE_FILES = $(wildcard src/command/*.cpp)

build/bin/runtests: tests/RunTests.cpp src/command/* tests/command/*
	g++ -std=c++17 -Wfatal-errors -I../farb/src/core -I../farb/src/interface -I../farb/src/reflection -I../farb/src/serialization -I../farb/src/utils tests/RunTests.cpp $(SOURCE_FILES) ../farb/build/link/farb.a -g -o ./build/bin/runtests