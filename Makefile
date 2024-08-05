# Makefile for compiling train_handler and main

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Executable name
TARGET = train_manager

# Source files
SRCS = main.c train_handler.c

# Object files
OBJS = $(SRCS:.c=.o)

# Phony targets
.PHONY: all clean

# Default target
all: $(TARGET)

# Link object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile each source file into an object file
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Clean up object files and the executable
clean:
	rm -f $(OBJS) $(TARGET)
