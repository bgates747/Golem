CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O2
PROJECT := $(abspath ../../..)
BUILD := build

.PHONY: all check clean
all: $(BUILD)/golemc-integrated
$(BUILD)/golemc-integrated: golemc_integrated.cpp integration.hpp scalar_backend.hpp $(PROJECT)/src/golemc.cpp $(PROJECT)/src/token_reader.hpp $(PROJECT)/src/buffer_allocator.hpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@
check:
	$(PROJECT)/.venv/bin/python run_checks.py
clean:
	rm -rf $(BUILD)
