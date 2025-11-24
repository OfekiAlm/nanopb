# Quick Start: Writing Nanopb Tests

This guide gets you writing tests quickly with minimal explanation. For detailed information, see `TESTING_GUIDE.md` and `SITE_SCONS_REFERENCE.md`.

## Prerequisites

```bash
cd tests
pip3 install scons protobuf grpcio-tools
```

## Test Template

### 1. Create Directory

```bash
mkdir my_test
cd my_test
```

### 2. Create Protocol Definition

**my_message.proto**:
```protobuf
syntax = "proto2";

import "nanopb.proto";

message MyMessage {
    required int32 id = 1;
    optional string name = 2 [(nanopb).max_size = 40];
}
```

**my_message.options** (optional):
```
# Control code generation
MyMessage.name max_size:64
```

### 3. Create Encoder

**encode_test.c**:
```c
#include <stdio.h>
#include <pb_encode.h>
#include "my_message.pb.h"
#include "test_helpers.h"

int main(int argc, char **argv) {
    uint8_t buffer[MyMessage_size];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    
    MyMessage msg = MyMessage_init_zero;
    msg.id = 42;
    msg.has_name = true;
    strcpy(msg.name, "test");
    
    if (pb_encode(&stream, MyMessage_fields, &msg)) {
        SET_BINARY_MODE(stdout);
        fwrite(buffer, 1, stream.bytes_written, stdout);
        return 0;
    }
    
    fprintf(stderr, "Encoding failed: %s\n", PB_GET_ERROR(&stream));
    return 1;
}
```

### 4. Create Decoder

**decode_test.c**:
```c
#include <stdio.h>
#include <pb_decode.h>
#include "my_message.pb.h"
#include "test_helpers.h"

int main(int argc, char **argv) {
    uint8_t buffer[MyMessage_size];
    MyMessage msg = MyMessage_init_zero;
    
    SET_BINARY_MODE(stdin);
    size_t count = fread(buffer, 1, sizeof(buffer), stdin);
    
    pb_istream_t stream = pb_istream_from_buffer(buffer, count);
    
    if (pb_decode(&stream, MyMessage_fields, &msg)) {
        printf("id: %d\n", msg.id);
        if (msg.has_name) {
            printf("name: %s\n", msg.name);
        }
        return 0;
    }
    
    fprintf(stderr, "Decoding failed: %s\n", PB_GET_ERROR(&stream));
    return 1;
}
```

### 5. Create Build File

**SConscript**:
```python
Import("env")

# Generate .pb.c and .pb.h from .proto
env.NanopbProto(["my_message", "my_message.options"])

# Build encoder
enc = env.Program([
    "encode_test.c",
    "my_message.pb.c",
    "$COMMON/pb_encode.o",
    "$COMMON/pb_common.o"
])

# Build decoder
dec = env.Program([
    "decode_test.c",
    "my_message.pb.c",
    "$COMMON/pb_decode.o",
    "$COMMON/pb_common.o"
])

# Run encoder → encode_test.output
env.RunTest(enc)

# Run decoder with encoder output as input → decode_test.output
env.RunTest([dec, "encode_test.output"])

# Decode binary with protoc → encode_test.decoded
env.Decode(["encode_test.output", "my_message.proto"], MESSAGE="MyMessage")

# Compare nanopb decoder with protoc decoder
env.Compare(["decode_test.output", "encode_test.decoded"])
```

### 6. Run Test

```bash
cd ..  # Back to tests/
scons my_test
```

## Common Patterns

### Testing Multiple Scenarios

```python
# Default behavior
env.RunTest(enc)

# With command-line argument
env.RunTest("scenario1.output", enc, ARGS=['1'])
env.RunTest("scenario2.output", enc, ARGS=['2'])

# Different decoders for each
env.RunTest("scenario1.decout", [dec, "scenario1.output"])
env.RunTest("scenario2.decout", [dec, "scenario2.output"])
```

### Pattern Matching Instead of Exact Comparison

**expected_output.txt**:
```
id: \d+
name: \w+
! ERROR
! failed
```

**SConscript**:
```python
env.RunTest(program)
env.Match(["program.output", "expected_output.txt"])
```

### Testing Round-Trip Encoding

```python
# Encode with nanopb
env.RunTest(enc)

# Decode with protoc
env.Decode("encode_test.output.decoded", 
           ["encode_test.output", "my_message.proto"], 
           MESSAGE="MyMessage")

# Re-encode with protoc
env.Encode("encode_test.output.recoded",
           ["encode_test.output.decoded", "my_message.proto"],
           MESSAGE="MyMessage")

# Verify byte-for-byte identical
env.Compare(["encode_test.output", "encode_test.output.recoded"])
```

### Using Common Test Messages

Instead of creating your own proto:

```python
Import("env")

# Use person.proto from common/
enc = env.Program([
    "encode_person.c",
    "$COMMON/person.pb.c",  # Already generated
    "$COMMON/pb_encode.o",
    "$COMMON/pb_common.o"
])

env.RunTest(enc)
```

## Builder Reference

### NanopbProto

Generates `.pb.c` and `.pb.h` from `.proto`:

```python
env.NanopbProto("message")
# Creates: message.pb.c, message.pb.h

env.NanopbProto(["message", "message.options"])
# Uses options file for generation
```

### RunTest

Runs a program and captures stdout:

```python
# Basic
env.RunTest(program)
# → program.output

# With input file
env.RunTest([program, "input.bin"])
# → program.output (reads input.bin on stdin)

# With arguments
env.RunTest(program, ARGS=['--verbose', '42'])
# → program.output

# Custom output name
env.RunTest("custom.output", program)
# → custom.output
```

### Decode

Decodes binary protobuf with protoc:

```python
env.Decode(["binary_file.bin", "schema.proto"], MESSAGE="MessageType")
# → binary_file.decoded
```

### Encode

Encodes text protobuf with protoc:

```python
env.Encode(["text_file.txt", "schema.proto"], MESSAGE="MessageType")
# → text_file.encoded
```

### Compare

Asserts files are identical:

```python
env.Compare([file1, file2])
# Fails build if files differ
```

### Match

Checks regex patterns:

```python
env.Match([output_file, patterns_file])
# Patterns file: one regex per line
# Lines starting with "! " are inverted (must NOT match)
```

## Common Paths

| Variable | Path | Description |
|----------|------|-------------|
| `$COMMON` | `build/common` | Shared resources |
| `$COMMON/pb_encode.o` | | Nanopb encoder |
| `$COMMON/pb_decode.o` | | Nanopb decoder |
| `$COMMON/pb_common.o` | | Nanopb utilities |
| `$COMMON/person.pb.c` | | Example message |
| `$NANOPB` | `../` | Nanopb root |

## Test Helpers

Include `test_helpers.h` for utilities:

```c
#include "test_helpers.h"

// Set binary mode for stdin/stdout (Windows compatibility)
SET_BINARY_MODE(stdin);
SET_BINARY_MODE(stdout);

// Get error message from stream
fprintf(stderr, "Error: %s\n", PB_GET_ERROR(&stream));
```

## Running Tests

```bash
# All tests
scons

# Specific test
scons my_test

# Multiple tests
scons basic_buffer alltypes

# Clean
scons -c

# Parallel build (4 jobs)
scons -j4

# Verbose
scons --debug=explain

# Different compiler
scons CC=clang CXX=clang++
```

## Cross-Platform Testing

```bash
# Native (default)
scons

# AVR microcontroller (simulated)
scons PLATFORM=AVR

# ARM Cortex-M (STM32)
scons PLATFORM=STM32

# MIPS
scons PLATFORM=MIPS
```

## Troubleshooting

### "protoc: command not found"

```bash
pip3 install grpcio-tools
```

### "Could not find nanopb root"

Make sure you're in the `tests/` directory when running scons.

### "No rule to make target"

```bash
scons -c  # Clean and retry
```

### Test Fails but Should Pass

Check file contents:

```bash
cat build/my_test/encode_test.output | xxd
cat build/my_test/decode_test.output
```

Compare manually:

```bash
diff build/my_test/decode_test.output build/my_test/encode_test.decoded
```

### Generated Files Not Found

Make sure `NanopbProto` is called before `Program`:

```python
env.NanopbProto("message")  # Must come first
enc = env.Program(["encode.c", "message.pb.c", ...])
```

## Advanced Examples

### Multiple Message Types

```python
env.NanopbProto("messages")  # messages.proto contains multiple types

# Test Message1
enc1 = env.Program(["encode_msg1.c", "messages.pb.c", ...])
env.RunTest(enc1)
env.Decode(["encode_msg1.output", "messages.proto"], MESSAGE="Message1")

# Test Message2
enc2 = env.Program(["encode_msg2.c", "messages.pb.c", ...])
env.RunTest(enc2)
env.Decode(["encode_msg2.output", "messages.proto"], MESSAGE="Message2")
```

### Callbacks

```python
enc = env.Program([
    "encode_callback.c",
    "message.pb.c",
    "$COMMON/pb_encode.o",
    "$COMMON/pb_common.o"
])

# Callbacks may produce different output each time
# Use pattern matching instead of exact comparison
env.RunTest(enc)
env.Match(["encode_callback.output.txt", "expected_patterns.txt"])
```

### Memory Testing with Malloc Support

```python
Import("env", "malloc_env")

# Use malloc_env for tests that need dynamic allocation
enc = malloc_env.Program([
    "encode_malloc.c",
    "message.pb.c",
    "$COMMON/pb_encode_with_malloc.o",
    "$COMMON/pb_common_with_malloc.o",
    "$COMMON/malloc_wrappers.o"
])

env.RunTest(enc)
```

### C++ Tests

```python
env.NanopbProtoCpp("message")  # Generates .pb.cpp and .pb.hpp

cxx_prog = env.Program([
    "test.cpp",
    "message.pb.cpp",
    "$COMMON/pb_encode.o",
    "$COMMON/pb_common.o"
])

env.RunTest(cxx_prog)
```

## Next Steps

- Read `TESTING_GUIDE.md` for comprehensive documentation
- Read `SITE_SCONS_REFERENCE.md` for technical details
- Look at existing tests in `tests/` for more examples
- Explore `common/SConscript` to understand shared resources

## Minimal Working Example

For the absolute minimum test:

**tests/minimal_test/SConscript**:
```python
Import("env")

# Use existing person.proto from common/
enc = env.Program(["encode_person.c", "$COMMON/person.pb.c",
                   "$COMMON/pb_encode.o", "$COMMON/pb_common.o"])
dec = env.Program(["decode_person.c", "$COMMON/person.pb.c",
                   "$COMMON/pb_decode.o", "$COMMON/pb_common.o"])

env.RunTest(enc)
env.RunTest([dec, "encode_person.output"])
```

Copy `encode_buffer.c` and `decode_buffer.c` from `basic_buffer/`, rename to `encode_person.c` and `decode_person.c`.

Run:
```bash
scons minimal_test
```

That's it! You now have a working test.
