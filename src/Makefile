# Makefile for golemc, the host Golem compiler.
#
# This builds the compiler itself (a Linux host-side tool), not any
# Golem-compiled program output. See docs/devlog/2026-07-28-compiler-toolchain.md
# for the host language/toolchain decision this follows.
#
# Usage:
#   make            # builds ../build/golemc
#   make clean

CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2

BUILD_DIR := ../build
BIN := $(BUILD_DIR)/golemc

.PHONY: all check clean

all: $(BIN)

$(BIN): golemc.cpp assembly_output.hpp resident_drawing.hpp resident_table.hpp ordinary.hpp resident.hpp scalar_backend.hpp token_reader.hpp buffer_allocator.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/test_compiler: ../tests/test_compiler.cpp assembly_output.hpp resident_drawing.hpp resident_table.hpp ordinary.hpp resident.hpp scalar_backend.hpp token_reader.hpp buffer_allocator.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

check: $(BIN) $(BUILD_DIR)/test_compiler
	$(BUILD_DIR)/test_compiler

clean:
	rm -f $(BIN) $(BUILD_DIR)/test_compiler
