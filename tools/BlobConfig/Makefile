# Toolchain/Environment
export SHELL := /bin/bash
export CC    := g++

# Dependency: MetroUtil
METROUTIL_DIR := ../../MetroUtil
METROUTIL_LIB := $(METROUTIL_DIR)/lib/libMetrobotics.a

# Dependency: Surveyor
SURVEYOR_DIR  := ../../src/Surveyor/
SURVEYOR_HDR := $(shell find $(SURVEYOR_DIR) -type f -name "*.h")
SURVEYOR_SRC := $(shell find $(SURVEYOR_DIR) -type f -name "*.cpp")

# Options
CFLAGS := -Wall -I$(METROUTIL_DIR)/include -I$(SURVEYOR_DIR)
LIBS   := -L$(METROUTIL_DIR)/lib -lMetrobotics -lSDL -lSDL_image -lSDL_gfx

# Debugging support
export DEBUG ?= 1
ifeq ($(DEBUG), 0)
	CFLAGS := $(CFLAGS) -g0
else
	CFLAGS := $(CFLAGS) -g3
endif

# General targets
.PHONY: BlobConfig clean

# Default target
BlobConfig: main.cpp $(METROUTIL_LIB) $(SURVEYOR_SRC) $(SURVEYOR_HDR)
	$(CC) $(CFLAGS) -o BlobConfig main.cpp $(SURVEYOR_SRC) $(LIBS)

$(METROUTIL_LIB):
	@echo Building MetroUtil
	@$(MAKE) -e --directory="$(METROUTIL_DIR)" install
	@if [ "$$?" != 0 ]; then \
		echo "MetroUtil failed to build"; \
		echo "BlobConfig depends on MetroUtil"; \
		exit 1; \
	fi

clean:
	rm -rf BlobConfig
