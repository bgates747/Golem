CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O2
PROJECT := $(abspath ../../..)
BUILD := build

.PHONY: all check clean
all: $(BUILD)/golemc-scalar
$(BUILD)/golemc-scalar: golemc_scalar.cpp scalar_frontend.hpp ../GOLEM-003/scalar_backend.hpp $(PROJECT)/src/token_reader.hpp
	mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $< -o $@
check:
	$(PROJECT)/.venv/bin/python run_checks.py
clean:
	rm -rf $(BUILD)
