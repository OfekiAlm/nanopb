# Makefile for the nanopb validation simple example

NANOPB_DIR = ../..
VALIDATE_PROTO = $(NANOPB_DIR)/generator/proto/validate.proto

# Include nanopb basic rules
include $(NANOPB_DIR)/extra/nanopb.mk

# Compiler flags
CFLAGS = -Wall -Werror -g -O0
CFLAGS += -I$(NANOPB_DIR)

# Enable validation in the generator
PROTOC_OPTS += --nanopb_opt=--validate

# Source files
CSRC = validation_simple.c \
       validation_simple.pb.c \
       validation_simple_validate.c \
       $(NANOPB_DIR)/pb_encode.c \
       $(NANOPB_DIR)/pb_decode.c \
       $(NANOPB_DIR)/pb_common.c \
       $(NANOPB_DIR)/pb_validate.c

# Default target
all: validation_simple

# Clean target
clean:
	rm -f validation_simple
	rm -f *.pb.c *.pb.h *_validate.c *_validate.h
	rm -f *.o

# Generate the nanopb files with validation
validation_simple.pb.c validation_simple.pb.h validation_simple_validate.c validation_simple_validate.h: validation_simple.proto $(VALIDATE_PROTO)
	$(PROTOC) $(PROTOC_OPTS) --nanopb_out=. validation_simple.proto

# Build the example
validation_simple: $(CSRC)
	$(CC) $(CFLAGS) -o $@ $^

# Run the example
run: validation_simple
	./validation_simple

.PHONY: all clean run
