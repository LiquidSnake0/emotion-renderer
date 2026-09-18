# g++ seul, aucune dependance : c'est voulu tant que rien ne dessine.
CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -Wpedantic
SRC := src/main.cpp
HDR := $(wildcard src/*.hpp)

bin/emotion-renderer: $(SRC) $(HDR)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

.PHONY: clean
clean:
	rm -rf bin
