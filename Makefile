CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -g -pthread -MMD -MP
INC = -I include

# CLI: all .cpp files except daemon.cpp and benchmark.cpp
CLI_SRC = $(filter-out src/daemon.cpp src/benchmark.cpp, $(wildcard src/*.cpp))
CLI_OBJ = $(patsubst src/%.cpp, build/cli_%.o, $(CLI_SRC))
CLI_DEP = $(CLI_OBJ:.o=.d)

# Daemon: all .cpp files except main.cpp and benchmark.cpp
DAEMON_SRC = $(filter-out src/main.cpp src/benchmark.cpp, $(wildcard src/*.cpp))
DAEMON_OBJ = $(patsubst src/%.cpp, build/daemon_%.o, $(DAEMON_SRC))
DAEMON_DEP = $(DAEMON_OBJ:.o=.d)

CLI_OUT = bin/indxr
DAEMON_OUT = bin/indxr-daemon

all: $(CLI_OUT)

daemon: $(DAEMON_OUT)

$(CLI_OUT): $(CLI_OBJ)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(CLI_OBJ) -o $(CLI_OUT)

$(DAEMON_OUT): $(DAEMON_OBJ)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(DAEMON_OBJ) -o $(DAEMON_OUT)

# CLI objects
build/cli_%.o: src/%.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

# Daemon objects
build/daemon_%.o: src/%.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) $(INC) -c $< -o $@

BENCH_SRC = src/benchmark.cpp $(filter-out src/main.cpp src/benchmark.cpp src/daemon.cpp, $(wildcard src/*.cpp))
benchmark:
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -O2 $(INC) $(BENCH_SRC) -o bin/benchmark

run-bench: benchmark
	./bin/benchmark $(ARGS)

run: all
	./$(CLI_OUT) $(ARGS)

run-daemon: daemon
	./$(DAEMON_OUT) $(ARGS)

clean:
	rm -rf build bin

-include $(CLI_DEP) $(DAEMON_DEP)