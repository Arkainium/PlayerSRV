# Toolchain/Environment
SHELL := /bin/bash
CC    := g++

# Options
CFLAGS := -Wall -fPIC -c -I../../MetroUtil/include

# Debugging support
DEBUG  ?= 1
ifeq ($(DEBUG), 0)
	CFLAGS := $(CFLAGS) -g0
else
	CFLAGS := $(CFLAGS) -g3
endif

# Files
OUTPUT  := Surveyor.a
SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp, %.o, $(SOURCES))
HEADERS := $(wildcard *.h)

# General targets
.PHONY: clean

# Default target
$(OUTPUT): $(OBJECTS)
	ar rs $(OUTPUT) $(OBJECTS)

# Remove unnecessary output files
clean:
	rm -rf $(OUTPUT) $(OBJECTS)

# Individual source targets
Surveyor.o: Surveyor.cpp Surveyor.h Picture.h IRArray.h YUVRange.h Rect.h
	$(CC) -c $(CFLAGS) Surveyor.cpp
