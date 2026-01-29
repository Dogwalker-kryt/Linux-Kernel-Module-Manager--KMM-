CC := gcc
CFLAGS := -Wall -Wextra -O2 -fPIC
INCLUDES := -I./include
LIBS := -lkmod
LDFLAGS := $(LIBS)

TARGET := kmm
SRC_DIR := src
OBJ_DIR := build
INCLUDE_DIR := include

SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJECTS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SOURCES))

.PHONY: all clean build install help

all: $(TARGET)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(TARGET): $(OBJECTS)
	@echo "[LD] Linking $@"
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "[OK] Built: $@"

clean:
	@echo "[CLEAN] Removing build artifacts"
	@rm -rf $(OBJ_DIR) $(TARGET)
	@echo "[OK] Cleaned"

build: clean all

help:
	@echo "KMM (Kernel Module Manager) - Build targets:"
	@echo "  make all     - Build the project (default)"
	@echo "  make clean   - Remove build artifacts"
	@echo "  make build   - Clean and rebuild"
	@echo "  make help    - Show this help"

# Dependencies check
check-deps:
	@which pkg-config > /dev/null || (echo "Error: pkg-config not found"; exit 1)
	@pkg-config --exists libkmod || (echo "Error: libkmod not found. Install with: sudo apt install libkmod-dev"; exit 1)
	@echo "[OK] All dependencies found"
