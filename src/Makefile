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
PORTABLE_FLAGS := -ffp-contract=off
PORTABLE_HEADERS := load_plan.hpp portable/driver.hpp portable/root.hpp portable/backends.hpp portable/portable.hpp

BUILD_DIR := ../build
BIN := $(BUILD_DIR)/golemc

.PHONY: all check clean

all: $(BIN)

$(BIN): load_plan.hpp stock_matrix.hpp golemc.cpp assembly_output.hpp resident_drawing.hpp resident_table.hpp ordinary.hpp resident.hpp scalar_backend.hpp token_reader.hpp buffer_allocator.hpp $(PORTABLE_HEADERS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(PORTABLE_FLAGS) -o $@ golemc.cpp

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/test_compiler: ../tests/test_compiler.cpp assembly_output.hpp resident_drawing.hpp resident_table.hpp ordinary.hpp resident.hpp scalar_backend.hpp token_reader.hpp buffer_allocator.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BUILD_DIR)/test_portable: ../tests/test_portable.cpp $(PORTABLE_HEADERS) ordinary.hpp token_reader.hpp buffer_allocator.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(PORTABLE_FLAGS) -o $@ $<

$(BUILD_DIR)/test_stock_matrix: ../tests/test_stock_matrix.cpp stock_matrix.hpp assembly_output.hpp resident_drawing.hpp resident_table.hpp ordinary.hpp resident.hpp scalar_backend.hpp token_reader.hpp buffer_allocator.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(PORTABLE_FLAGS) -o $@ $<

$(BUILD_DIR)/test_stock_loops: ../tests/test_stock_loops.cpp stock_matrix.hpp assembly_output.hpp resident_drawing.hpp resident_table.hpp ordinary.hpp resident.hpp scalar_backend.hpp token_reader.hpp buffer_allocator.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(PORTABLE_FLAGS) -o $@ $<

$(BUILD_DIR)/test_bitwise: ../tests/test_bitwise.cpp $(PORTABLE_HEADERS) token_reader.hpp buffer_allocator.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(PORTABLE_FLAGS) -o $@ $<

$(BUILD_DIR)/test_load_plan: ../tests/test_load_plan.cpp load_plan.hpp scalar_backend.hpp buffer_allocator.hpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

check: $(BUILD_DIR)/test_bitwise $(BUILD_DIR)/test_load_plan $(BIN) $(BUILD_DIR)/test_compiler $(BUILD_DIR)/test_portable $(BUILD_DIR)/test_stock_matrix $(BUILD_DIR)/test_stock_loops
	$(BUILD_DIR)/test_bitwise
	$(BUILD_DIR)/test_load_plan
	$(BUILD_DIR)/test_compiler
	$(BUILD_DIR)/test_stock_matrix
	$(BUILD_DIR)/test_stock_loops
	$(BUILD_DIR)/test_portable ../tests/portable/fixtures $(BUILD_DIR)/test-portable-output

clean:
	rm -f $(BIN) $(BUILD_DIR)/test_bitwise $(BUILD_DIR)/test_load_plan $(BUILD_DIR)/test_compiler $(BUILD_DIR)/test_portable $(BUILD_DIR)/test_stock_matrix $(BUILD_DIR)/test_stock_loops
