# Toolchain/Environment
SHELL := /bin/bash
CC    := g++

# Options
CFLAGS := -Wall -fPIC -c -I../MetroUtil/include `pkg-config --cflags playercore`
LFLAGS := -Wall -Wl,-z,defs -shared
LIBS   := `pkg-config --libs playercore` -L../MetroUtil/lib -lMetrobotics -lboost_thread-mt

# Debugging support
DEBUG  ?= 1
ifeq ($(DEBUG), 0)
	CFLAGS := $(CFLAGS) -g0
else
	CFLAGS := $(CFLAGS) -g3
endif

# Files
OUTPUT    := libPlayerSRV.so
SOURCES   := $(wildcard *.cpp)
OBJECTS   := $(patsubst %.cpp, %.o, $(SOURCES))
HEADERS   := $(shell find ./ -type f -name "*.h")
DEPENDS   := Surveyor/Surveyor.a Interfaces/Interfaces.a Commands/Commands.a

# General targets
.PHONY: clean

# Default target: build the driver
PlayerSRV: $(OBJECTS) $(DEPENDS)
	$(CC) $(LFLAGS) -o $(OUTPUT) $(OBJECTS) $(DEPENDS) $(LIBS)

# Remove unnecessary output files
clean:
	@$(MAKE) -e --directory="./Surveyor" clean
	@$(MAKE) -e --directory="./Commands" clean
	@$(MAKE) -e --directory="./Interfaces" clean
	rm -rf $(OUTPUT) $(OBJECTS)

# Individual source targets
PlayerSRV.o: PlayerSRV.cpp $(HEADERS)
	$(CC) -c $(CFLAGS) PlayerSRV.cpp

# Dependencies
Surveyor/Surveyor.a: $(shell find ./Surveyor -type f -name "*.cpp")
	@$(MAKE) -e --directory="./Surveyor"

Commands/Commands.a: $(shell find ./Commands -type f -name "*.cpp")
	@$(MAKE) -e --directory="./Commands"

Interfaces/Interfaces.a: $(shell find ./Interfaces -type f -name "*.cpp")
	@$(MAKE) -e --directory="./Interfaces"
