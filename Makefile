# ============================================================
#  Makefile for the Huffman archiver
#  Standard: C99
# ============================================================

CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDFLAGS =

# ----------------------------------------------------------------
#  Directories
# ----------------------------------------------------------------
SRC_DIR   = src
INC_DIR   = include
TEST_DIR  = tests
BUILD_DIR = build

# ----------------------------------------------------------------
#  Source files and objects (library modules, no main)
# ----------------------------------------------------------------
LIB_SRCS = $(SRC_DIR)/pqueue.c   \
            $(SRC_DIR)/huffman.c  \
            $(SRC_DIR)/bitstream.c \
            $(SRC_DIR)/compress.c

LIB_OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(LIB_SRCS))

# ----------------------------------------------------------------
#  Main binary
# ----------------------------------------------------------------
TARGET = archiver

# ----------------------------------------------------------------
#  Test binaries
# ----------------------------------------------------------------
TEST_SRCS = $(TEST_DIR)/test_pqueue.c   \
            $(TEST_DIR)/test_huffman.c  \
            $(TEST_DIR)/test_bitstream.c \
            $(TEST_DIR)/test_compress.c

TEST_BINS = $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%, $(TEST_SRCS))

# ============================================================
#  Default target
# ============================================================
.PHONY: all
all: $(TARGET)

# ----------------------------------------------------------------
#  Create build directory
# ----------------------------------------------------------------
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ----------------------------------------------------------------
#  Compile library objects
# ----------------------------------------------------------------
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ----------------------------------------------------------------
#  Link main binary
# ----------------------------------------------------------------
$(TARGET): $(LIB_OBJS) $(BUILD_DIR)/main.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(BUILD_DIR)/main.o: main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# ----------------------------------------------------------------
#  Build and run all tests
# ----------------------------------------------------------------
.PHONY: test
test: $(TEST_BINS)
	@echo ""
	@echo "========================================="
	@echo " Running all tests"
	@echo "========================================="
	@FAILED=0; \
	for t in $(TEST_BINS); do \
	    echo ""; \
	    echo "--- $$t ---"; \
	    ./$$t || FAILED=$$((FAILED + 1)); \
	done; \
	echo ""; \
	if [ $$FAILED -eq 0 ]; then \
	    echo "All test suites passed."; \
	else \
	    echo "$$FAILED test suite(s) FAILED."; \
	    exit 1; \
	fi

$(BUILD_DIR)/test_%: $(TEST_DIR)/test_%.c $(LIB_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $< $(LIB_OBJS) -o $@ $(LDFLAGS)

# ----------------------------------------------------------------
#  Debug build (with AddressSanitizer)
# ----------------------------------------------------------------
.PHONY: debug
debug: CFLAGS += -g -fsanitize=address,undefined -fno-omit-frame-pointer
debug: LDFLAGS += -fsanitize=address,undefined
debug: clean all test

# ----------------------------------------------------------------
#  Clean
# ----------------------------------------------------------------
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# ----------------------------------------------------------------
#  Help
# ----------------------------------------------------------------
.PHONY: help
help:
	@echo "Targets:"
	@echo "  all      Build the archiver binary (default)"
	@echo "  test     Build and run all unit/integration tests"
	@echo "  debug    Build with AddressSanitizer + UBSan, run tests"
	@echo "  clean    Remove build artefacts"
