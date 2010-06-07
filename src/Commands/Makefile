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
OUTPUT  := Commands.a
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
Command.o: Command.cpp Command.h
	$(CC) -c $(CFLAGS) Command.cpp

SyncSRV.o: SyncSRV.cpp SyncSRV.h Command.h \
           ../PlayerSRV.h ../Surveyor/Surveyor.h
	$(CC) -c $(CFLAGS) SyncSRV.cpp

DriveSRV.o: DriveSRV.cpp DriveSRV.h Command.h \
            ../PlayerSRV.h ../Surveyor/Surveyor.h ../Interfaces/Position2D.h
	$(CC) -c $(CFLAGS) DriveSRV.cpp

TakePictureSRV.o: TakePictureSRV.cpp TakePictureSRV.h Command.h \
            ../PlayerSRV.h ../Surveyor/Surveyor.h ../Interfaces/Camera.h
	$(CC) -c $(CFLAGS) TakePictureSRV.cpp

BounceSRV.o: BounceSRV.cpp BounceSRV.h Command.h \
            ../PlayerSRV.h ../Surveyor/Surveyor.h
	$(CC) -c $(CFLAGS) BounceSRV.cpp
