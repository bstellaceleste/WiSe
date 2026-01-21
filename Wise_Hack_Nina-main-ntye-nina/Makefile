# Adaptive Binary Instrumentation Engine - Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g -D_GNU_SOURCE
LDFLAGS = -lm -ldl

# Directories
SRC_DIR = src
BUILD_DIR = build
TEST_DIR = tests

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.c)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Target executable
TARGET = adaptive_engine

# Default target
all: $(BUILD_DIR) $(TARGET)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Build target executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✓ Build complete: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
	rm -f results.json results_*.json
	@echo "✓ Clean complete"

# Run tests
test: $(TARGET)
	@echo "Running test suite..."
	@./tests/run_tests.sh

# Install dependencies (Debian/Ubuntu)
install-deps:
	@echo "Installing dependencies..."
	sudo apt-get update
	sudo apt-get install -y build-essential gcc make libdl-dev

# Help
help:
	@echo "Adaptive Binary Instrumentation Engine - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all           - Build the engine (default)"
	@echo "  clean         - Remove build artifacts"
	@echo "  test          - Run test suite"
	@echo "  install-deps  - Install dependencies (Debian/Ubuntu)"
	@echo "  help          - Show this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make                    # Build the engine"
	@echo "  make clean              # Clean build"
	@echo "  make test               # Run tests"

# Phony targets
.PHONY: all clean test install-deps help
