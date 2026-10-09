# Plain-make alternative to CMake:  make test
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O1 -Iinclude -Itests
CORE     := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
BUILD    := build-make

all: $(BUILD)/rrs $(BUILD)/unit_tests $(BUILD)/integration_tests

$(BUILD)/rrs: $(CORE) src/main.cpp
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $^ -o $@
$(BUILD)/unit_tests: $(CORE) tests/test_main.cpp $(wildcard tests/unit/*.cpp) $(wildcard tests/*.h)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(CORE) tests/test_main.cpp $(wildcard tests/unit/*.cpp) -o $@
$(BUILD)/integration_tests: $(CORE) tests/test_main.cpp $(wildcard tests/integration/*.cpp) $(wildcard tests/*.h)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) $(CORE) tests/test_main.cpp $(wildcard tests/integration/*.cpp) -o $@

test: all
	./$(BUILD)/unit_tests
	./$(BUILD)/integration_tests

clean:
	rm -rf $(BUILD) build data
.PHONY: all test clean
