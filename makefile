CXX=g++
CXXFLAGS=-std=c++17 -Wall -pedantic -Wfatal-errors -Wno-gnu-statement-expression -Wno-unused-function -ferror-limit=1
TARGET_LINKS=-framework OpenGL -framework Cocoa

COMMAND_SOURCE_FILES = $(wildcard src/command/*.cpp)

GAME_MODULES = app game
GAME_INCLUDES = $(addprefix -I src/, $(GAME_MODULES))
GAME_SOURCE_FILES = $(foreach MODULE,$(GAME_MODULES),$(wildcard src/$(MODULE)/*.cpp))

FARB_MODULES = core interface reflection serialization utils
FARB_LIBS = tigr json
FARB_INCLUDES = $(addprefix -I ../farb/src/, $(FARB_MODULES)) $(addprefix -I ../farb/lib/, $(FARB_LIBS))

all: build/bin/runtests build/bin/brushlink

build/bin/runtests: tests/RunTests.cpp src/command/* tests/command/* ../farb/build/link/farb.a
	g++ -std=c++17 -Wfatal-errors -ferror-limit=1 $(FARB_INCLUDES) tests/RunTests.cpp $(COMMAND_SOURCE_FILES) ../farb/build/link/farb.a -g -o ./build/bin/runtests

build/bin/brushlink: src/game/* src/app/* ../farb/build/link/farb.a
	$(CXX) $(CXXFLAGS) $(FARB_INCLUDES) $(GAME_INCLUDES) $(GAME_SOURCE_FILES) ../farb/build/link/farb.a -g -o ./build/bin/brushlink $(TARGET_LINKS)

stats:
	for module in $(GAME_MODULES) ; do \
		echo MODULE $$module ; \
    	cloc ./src/$$module ; \
	done