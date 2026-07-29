# Makefile for golemc, the toy Golem compiler.
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

.PHONY: all clean

all: $(BIN)

$(BIN): golemc.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -f $(BIN)
