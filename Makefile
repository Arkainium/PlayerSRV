# Toolchain/Environment
export SHELL := /bin/bash
export CC    := g++

# Debugging support
export DEBUG ?= 1

# General targets
.PHONY: PlayerSRV clean purge

# Default target: build the driver
PlayerSRV:
	@echo Building PlayerSRV
	@$(MAKE) -e --directory="./src"
	@if [ "$$?" == 0 ]; then \
		cp ./src/libPlayerSRV.so ./; \
	else \
		echo "PlayerSRV failed to build"; \
	fi

# Remove unnecessary output files
clean:
	@$(MAKE) -e --directory="./src" clean

# Remove all output files
purge:
	@$(MAKE) -e clean
	@rm -rf libPlayerSRV.so
