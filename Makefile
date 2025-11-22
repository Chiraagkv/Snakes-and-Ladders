# --- Makefile for Snakes and Ladders (using raylib) ---

# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lraylib -lm -ldl -lpthread -lrt -lX11

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# Source Files
SOURCES = $(wildcard $(SRC_DIR)/*.c)

# Object Files
OBJECTS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SOURCES))

# Executable Name
TARGET = snakes_and_ladders

.PHONY: all clean run

all: $(TARGET)

# Rule to link the final executable
$(TARGET): $(OBJECTS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "--- Compilation complete. Run with ./$(TARGET) ---"

# Rule to compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Clean up built files and directories
clean:
	rm -rf $(BUILD_DIR) $(TARGET) s_and_l_save.txt
	@echo "--- Cleaned up build files and save file ---"

# Run the executable
run: $(TARGET)
	./$(TARGET)