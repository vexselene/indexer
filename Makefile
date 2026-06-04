CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -g -pthread -MMD -MP
INC = -I include

SRC = $(filter-out src/benchmark.cpp, $(wildcard src/*.cpp))
OBJ = $(patsubst src/%.cpp, build/%.o, $(SRC))
DEP = $(OBJ:.o=.d)

OUT = bin/indxr

all: $(OUT)

$(OUT): $(OBJ)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(OUT)

build/%.o: src/%.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

BENCH_SRC = src/benchmark.cpp $(filter-out src/main.cpp src/benchmark.cpp, $(wildcard src/*.cpp))
benchmark:
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -O2 $(INC) $(BENCH_SRC) -o bin/benchmark

run-bench: benchmark
	./bin/benchmark $(ARGS)

run: all
	./$(OUT) $(ARGS)

clean:
	rm -rf build bin

-include $(DEP)