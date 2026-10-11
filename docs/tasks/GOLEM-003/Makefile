CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O2

.PHONY: all test
all: build/golemc-scalar

build/golemc-scalar: golemc_scalar.cpp scalar_frontend.hpp scalar_backend.hpp | build
	$(CXX) $(CXXFLAGS) golemc_scalar.cpp -o $@

build:
	mkdir -p $@

test:
	../../../.venv/bin/python run_checks.py
