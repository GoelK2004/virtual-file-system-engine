# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wuninitialized -O2 -g -D_GLIBCXX_USE_CXX11_ABI=1

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BIN_DIR = bin

# Source files
SRCS = $(shell find $(SRC_DIR) -name "*.cpp")
OBJS := $(patsubst $(SRC_DIR)/%.cpp, $(BIN_DIR)/%.o, $(SRCS))
$(info $(SRCS))

# Executable
TARGET = $(BIN_DIR)/filesystem.exe

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Compile source files into object files
$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@
	
# Create bin directory if it does not exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Clean compiled files
clean:
	rm -rf $(BIN_DIR)/*.o $(TARGET)