# Makefile for validation test

NANOPB_DIR = ../..
VALIDATE_PROTO = $(NANOPB_DIR)/generator/proto/validate.proto

# Include nanopb basics
include $(NANOPB_DIR)/extra/nanopb.mk

# C compiler settings
CFLAGS += -Wall -Werror -g -O0
CFLAGS += -I$(NANOPB_DIR)

# Enable validation in generator
PROTOC_OPTS += --nanopb_opt=--validate

# Source files
SRCS = test_validation.c \
       validation_test.pb.c \
       validation_test_validate.c \
       $(NANOPB_DIR)/pb_encode.c \
       $(NANOPB_DIR)/pb_decode.c \
       $(NANOPB_DIR)/pb_common.c \
       $(NANOPB_DIR)/pb_validate.c

# Target
all: test_validation

clean:
	rm -f test_validation validation_test.pb.* validation_test_validate.* *.o

# Generate proto files with validation
validation_test.pb.c validation_test.pb.h validation_test_validate.c validation_test_validate.h: validation_test.proto $(VALIDATE_PROTO)
	$(PROTOC) $(PROTOC_OPTS) --nanopb_out=. validation_test.proto

test_validation: $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

run_test: test_validation
	./test_validation

.PHONY: all clean run_test
