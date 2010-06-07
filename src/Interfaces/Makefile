# Toolchain/Environment
SHELL := /bin/bash
CC    := g++

# Options
CFLAGS := -Wall -fPIC -c -I../../MetroUtil/include `pkg-config --cflags playercore`

# Debugging support
DEBUG  ?= 1
ifeq ($(DEBUG), 0)
	CFLAGS := $(CFLAGS) -g0
else
	CFLAGS := $(CFLAGS) -g3
endif

# Files
OUTPUT  := Interfaces.a
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
Position2D.o: Position2D.cpp Position2D.h ../PlayerSRV.h ../Commands/DriveSRV.h
	$(CC) -c $(CFLAGS) Position2D.cpp

Camera.o: Camera.cpp Camera.h ../PlayerSRV.h ../Commands/TakePictureSRV.h
	$(CC) -c $(CFLAGS) Camera.cpp

Ranger.o: Ranger.cpp Ranger.h ../PlayerSRV.h ../Commands/BounceSRV.h
	$(CC) -c $(CFLAGS) Ranger.cpp
