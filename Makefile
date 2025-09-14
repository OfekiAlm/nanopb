# Makefile for nanopb validation example

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
INCLUDES = -I.
LIBS = 

# Source files
NANOPB_SOURCES = pb_common.c pb_encode.c pb_decode.c
GENERATED_SOURCES = simple_user.pb.c
MAIN_SOURCE = main.c

# Object files
NANOPB_OBJECTS = $(NANOPB_SOURCES:.c=.o)
GENERATED_OBJECTS = $(GENERATED_SOURCES:.c=.o)
MAIN_OBJECT = main.o

# Target executables
TARGET = validation_example
ADVANCED_TARGET = advanced_validation_example

# Default target
all: $(TARGET) $(ADVANCED_TARGET)

# Build the main executable
$(TARGET): $(MAIN_OBJECT) $(GENERATED_OBJECTS) $(NANOPB_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compile main.c
main.o: main.c simple_user.pb.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile simple_advanced_main.c
simple_advanced_main.o: simple_advanced_main.c simple_user_profile.pb.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Build the advanced executable
$(ADVANCED_TARGET): simple_advanced_main.o simple_user_profile.pb.o $(NANOPB_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# Compile generated protobuf files
simple_user.pb.o: simple_user.pb.c simple_user.pb.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

simple_user_profile.pb.o: simple_user_profile.pb.c simple_user_profile.pb.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile nanopb core files
pb_common.o: pb_common.c pb.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

pb_encode.o: pb_encode.c pb_encode.h pb.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

pb_decode.o: pb_decode.c pb_decode.h pb.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Generate protobuf files
simple_user.pb.h simple_user.pb.c: simple_user.proto simple_user.options
	python3 generator/nanopb_generator.py simple_user.proto --options-file=simple_user.options

simple_user_profile.pb.h simple_user_profile.pb.c: simple_user_profile.proto simple_user_profile.options
	python3 generator/nanopb_generator.py simple_user_profile.proto --options-file=simple_user_profile.options

# Clean up
clean:
	rm -f *.o $(TARGET) $(ADVANCED_TARGET) simple_user.pb.h simple_user.pb.c simple_user_profile.pb.h simple_user_profile.pb.c

# Run the examples
run: $(TARGET)
	./$(TARGET)

run-advanced: $(ADVANCED_TARGET)
	./$(ADVANCED_TARGET)

# Show help
help:
	@echo "Available targets:"
	@echo "  all            - Build both validation examples (default)"
	@echo "  clean          - Remove generated files and object files"
	@echo "  run            - Build and run the simple validation example"
	@echo "  run-advanced   - Build and run the advanced validation example"
	@echo "  help           - Show this help message"

.PHONY: all clean run run-advanced help
