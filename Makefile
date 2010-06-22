# Toolchain/Environment
export SHELL := /bin/bash
export CC    := g++

# Debugging support
export DEBUG ?= 1

# Dependency: MetroUtil
export METROUTIL := MetroUtil/lib/libMetrobotics.a

# General targets
.PHONY: PlayerSRV clean purge

# Default target: build the driver
PlayerSRV: $(METROUTIL)
	@echo Building PlayerSRV
	@$(MAKE) -e --directory="./src"
	@if [ "$$?" == 0 ]; then \
		cp ./src/libPlayerSRV.so ./; \
	else \
		echo "PlayerSRV failed to build"; \
		exit 1; \
	fi

$(METROUTIL):
	@echo Building MetroUtil
	@$(MAKE) -e --directory="./MetroUtil" install
	@if [ "$$?" != 0 ]; then \
		echo "MetroUtil failed to build"; \
		echo "PlayerSRV depends on MetroUtil"; \
		exit 1; \
	fi

# Remove unnecessary output files
clean:
	@$(MAKE) -e --directory="./MetroUtil" clean
	@$(MAKE) -e --directory="./src" clean

# Remove all output files
purge:
	@$(MAKE) -e --directory="./MetroUtil" purge
	@$(MAKE) -e clean
	@rm -rf libPlayerSRV.so
