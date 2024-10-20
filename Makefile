# Makefile for building the kjson library

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++20 -O3 -DNDEBUG -fPIC -I. -DKXVER=3

# Output file
TARGET = kjson.so

# Source files
SOURCES = json_serialisation.cpp kjson_utils.cpp

# Default target
all: $(TARGET)

# Build shared library
$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $(TARGET) -shared

# Clean target
clean:
	rm -f $(TARGET)
