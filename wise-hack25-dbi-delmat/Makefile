# WiSe Hack'25 - Makefile
# Adaptive DBI Engine

# ========== CONFIGURATION ==========
DYNAMORIO_HOME ?= $(HOME)/dynamorio/build

# Compiler and flags
CC = gcc
CFLAGS = -std=gnu11 -fPIC -shared -Wall -Wextra -O2 -g
CFLAGS += -DLINUX -DX86_64 -D_FILE_OFFSET_BITS=64

# Include paths
INC_DIRS = -I./include
INC_DIRS += -I$(DYNAMORIO_HOME)/include
INC_DIRS += -I$(DYNAMORIO_HOME)/ext/include

# Library paths
LIB_DIRS = -L$(DYNAMORIO_HOME)/lib64/release
LIB_DIRS += -L$(DYNAMORIO_HOME)/ext/lib64/release

# Libraries to link
LIBS = -ldynamorio -ldrmgr -ldrwrap -lpthread -ldl

# Linker flags
LDFLAGS = $(LIB_DIRS) $(LIBS)

# ========== SOURCE FILES ==========
SRC_DIR = src
INC_DIR = include
SAMPLE_DIR = ../00-subject/sources
TEST_DIR = ../00-subject/binairies

SOURCES = $(SRC_DIR)/dbi_engine.c \
          $(SRC_DIR)/config.c \
          $(SRC_DIR)/hotpatch.c \
          $(SRC_DIR)/utils.c

OBJECTS = $(SOURCES:.c=.o)
HEADERS = $(wildcard $(INC_DIR)/*.h)

# Target library
TARGET = libwise_dbi.so

# Test binaries
TEST_BINS = $(TEST_DIR)/wise_memory \
            $(TEST_DIR)/wise_logic \
            $(TEST_DIR)/wise_balanced

# ========== PHONY TARGETS ==========
.PHONY: all clean samples test help run-memory run-logic run-balanced \
        check-dynamorio clean-samples copy-samples test-verbose debug

# ========== DEFAULT TARGET ==========
all: check-dynamorio $(TARGET)

# ========== CHECK DYNAMORIO ==========
check-dynamorio:
	@echo "Checking DynamoRIO installation..."
	@if [ ! -d "$(DYNAMORIO_HOME)" ]; then \
		echo "❌ ERROR: DynamoRIO not found at $(DYNAMORIO_HOME)"; \
		echo "Please install DynamoRIO or set DYNAMORIO_HOME"; \
		exit 1; \
	fi
	@if [ ! -f "$(DYNAMORIO_HOME)/lib64/release/libdynamorio.so" ]; then \
		echo "❌ ERROR: libdynamorio.so not found"; \
		exit 1; \
	fi
	@echo "✅ DynamoRIO found at $(DYNAMORIO_HOME)"

# ========== BUILD ENGINE ==========
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) -shared -o $@ $^ $(LDFLAGS)
	@echo "✅ $(TARGET) built successfully"
	@ls -lh $(TARGET)

# Pattern rule for object files
%.o: %.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(INC_DIRS) -c $< -o $@

# ========== BUILD TEST BINARIES ==========
samples: clean-samples
	@echo "========================================="
	@echo "Building test binaries..."
	@echo "========================================="
	@mkdir -p $(TEST_DIR)
	@echo "[1/4] Compiling wise_memory..."
	@gcc -O0 -g  -rdynamic -o $(TEST_DIR)/wise_memory $(SAMPLE_DIR)/source_1_memory.c
	@echo "      ✅ wise_memory compiled"
	@echo "[2/4] Compiling wise_logic..."
	@gcc -O0 -g -rdynamic -o $(TEST_DIR)/wise_logic $(SAMPLE_DIR)/source_2_logic.c
	@echo "      ✅ wise_logic compiled"
	@echo "[3/4] Compiling wise_balanced..."
	@gcc -O0 -g -rdynamic -o $(TEST_DIR)/wise_balanced $(SAMPLE_DIR)/source_3_balanced.c -lm
	@echo "      ✅ wise_balanced compiled"
	@echo "[4/4] Compiling wise_balanced..."
	@gcc -O0 -g -rdynamic -o $(TEST_DIR)/mystery $(SAMPLE_DIR)/source_4_final.c -lm
	@echo "      ✅ mystery compiled"
	@echo "========================================="
	@echo "Test binaries:"
	@ls -lh $(TEST_DIR)/
	@echo "========================================="

clean-samples:
	@rm -f $(TEST_DIR)/wise_*

# Copy samples to working directory (for easy access)
copy-samples: samples
	@mkdir -p samples
	@cp -f $(TEST_DIR)/* samples/ 2>/dev/null || true

# ========== TESTS ==========
test: $(TARGET) samples
	@echo "========================================="
	@echo "Running quick test (3 seconds)..."
	@echo "========================================="
	@DBI_VERBOSE=1 timeout 3 drrun -c ./$(TARGET) -- $(TEST_DIR)/wise_memory 2>&1 | \
		grep -E "ENGINE|Module loaded|Found function|Hotspot|Redirecting|HOT-PATCH|Successfully" | head -10 || true
	@echo "========================================="
	@echo "Test completed. For full output, use:"
	@echo "  make run-memory"
	@echo "========================================="

test-verbose: $(TARGET) samples
	@echo "Running verbose test (10 seconds)..."
	DBI_VERBOSE=1 timeout 10 drrun -c ./$(TARGET) -- $(TEST_DIR)/wise_memory

# Individual test runs
run-memory: $(TARGET) samples
	@echo "========================================="
	@echo "Running: wise_memory (Memory Intensive)"
	@echo "========================================="
	DBI_VERBOSE=1 $(DYNAMORIO_HOME)/bin64/drrun -root $(DYNAMORIO_HOME) -c ./$(TARGET) -- $(TEST_DIR)/wise_memory

run-logic: $(TARGET) samples
	@echo "========================================="
	@echo "Running: wise_logic (Logic Complexity)"
	@echo "========================================="
	DBI_VERBOSE=1 $(DYNAMORIO_HOME)/bin64/drrun -root $(DYNAMORIO_HOME) -c ./$(TARGET) -- $(TEST_DIR)/wise_logic

run-balanced: $(TARGET) samples
	@echo "========================================="
	@echo "Running: wise_balanced (Balanced Case)"
	@echo "========================================="
	DBI_VERBOSE=1 $(DYNAMORIO_HOME)/bin64/drrun -root $(DYNAMORIO_HOME) -c ./$(TARGET) -- $(TEST_DIR)/wise_balanced

run-mystery: $(TARGET) samples
	@echo "========================================="
	@echo "Running: mystery (Mystery Case)"
	@echo "========================================="
	DBI_VERBOSE=1 $(DYNAMORIO_HOME)/bin64/drrun -root $(DYNAMORIO_HOME) -c ./$(TARGET) -- $(TEST_DIR)/mystery

# Run all tests in sequence
test-all: $(TARGET) samples
	@echo "========================================="
	@echo "Running ALL tests..."
	@echo "========================================="
	@for bin in wise_memory wise_logic wise_balanced; do \
		echo ""; \
		echo ">>> Testing $$bin..."; \
		timeout 10 drrun -c ./$(TARGET) -- $(TEST_DIR)/$$bin 2>&1 | \
			grep -E "HOT-PATCH|Redirecting|Successfully|ERROR" || true; \
		echo "---"; \
	done
	@echo "========================================="
	@echo "All tests completed"
	@echo "========================================="

# ========== DEBUGGING ==========
debug: CFLAGS += -DDEBUG -O0
debug: clean all
	@echo "Built with debug symbols"

# Check for memory leaks (requires valgrind)
valgrind: $(TARGET) samples
	valgrind --leak-check=full drrun -c ./$(TARGET) -- $(TEST_DIR)/wise_memory

# ========== CLEAN ==========
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(OBJECTS)
	rm -f $(TARGET)
	rm -f $(TEST_DIR)/*
	@echo "✅ Clean complete"

clean-all: clean
	rm -rf $(TEST_DIR)

# ========== HELP ==========
help:
	@echo "========================================="
	@echo "WiSe Hack'25 - Adaptive DBI Engine"
	@echo "========================================="
	@echo ""
	@echo "Available targets:"
	@echo ""
	@echo "  make all              - Build the DBI engine (default)"
	@echo "  make samples          - Compile test binaries"
	@echo "  make test             - Quick test (3 seconds)"
	@echo "  make test-verbose     - Verbose test (10 seconds)"
	@echo "  make test-all         - Run all tests in sequence"
	@echo ""
	@echo "  make run-memory       - Test memory intensive binary"
	@echo "  make run-logic        - Test logic complexity binary"
	@echo "  make run-balanced     - Test balanced case binary"
	@echo ""
	@echo "  make clean            - Remove build artifacts"
	@echo "  make clean-all        - Remove everything"
	@echo "  make debug            - Build with debug symbols"
	@echo "  make help             - Show this help"
	@echo ""
	@echo "Configuration:"
	@echo "  DYNAMORIO_HOME = $(DYNAMORIO_HOME)"
	@echo ""
	@echo "Environment variables:"
	@echo "  DBI_VERBOSE=1         - Enable detailed logging"
	@echo "  DBI_HOTSPOT_MULT=1.5  - Adjust hotspot threshold"
	@echo "  DBI_MIN_CALLS=30      - Minimum calls before analysis"
	@echo "  DBI_TRACK_MEMORY=0    - Disable memory tracking"
	@echo ""
	@echo "Example:"
	@echo "  DBI_VERBOSE=1 DBI_HOTSPOT_MULT=1.5 make run-memory"
	@echo ""
	@echo "========================================="

# ========== INFO ==========
info:
	@echo "========================================="
	@echo "Build Information"
	@echo "========================================="
	@echo "Compiler:        $(CC)"
	@echo "CFLAGS:          $(CFLAGS)"
	@echo "DynamoRIO:       $(DYNAMORIO_HOME)"
	@echo "Target:          $(TARGET)"
	@echo "Sources:         $(SOURCES)"
	@echo "========================================="