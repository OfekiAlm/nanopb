# Nanopb Test Architecture Overview

This document provides a high-level architectural view of the nanopb test system.

## System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         User Interface                          │
│                                                                 │
│  $ scons [options] [targets]                                   │
│                                                                 │
│  Options:                                                       │
│    PLATFORM=AVR/STM32/MIPS    Cross-compilation                │
│    CC=compiler                Compiler selection               │
│    BUILDDIR=path              Output directory                 │
│    NOVALGRIND=1              Disable memory checking           │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                        SCons Core                               │
│                                                                 │
│  • Dependency tracking                                          │
│  • Incremental builds                                           │
│  • Parallel execution                                           │
│  • Configuration management                                     │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│                   SConstruct (tests/SConstruct)                 │
│                                                                 │
│  1. Create environment                                          │
│  2. Platform detection/configuration                            │
│  3. Compiler detection/configuration                            │
│  4. Load custom tools and builders                              │
│  5. Discover and process all SConscript files                   │
└──────┬─────────────────────┬────────────────────────┬───────────┘
       │                     │                        │
       ▼                     ▼                        ▼
┌──────────────┐   ┌──────────────────┐   ┌─────────────────────┐
│  site_scons/ │   │  common/         │   │  Test Directories   │
│              │   │  SConscript      │   │  (70+ tests)        │
│  Custom      │   │                  │   │                     │
│  Extensions  │   │  Build shared    │   │  Each contains:     │
└──────┬───────┘   │  resources:      │   │  • SConscript       │
       │           │  • pb_encode.o   │   │  • .proto files     │
       │           │  • pb_decode.o   │   │  • .c/.cpp files    │
       │           │  • person.pb.c   │   │  • .options files   │
       │           │  • unittest.pb.c │   └──────────┬──────────┘
       │           └─────────┬────────┘              │
       │                     │                        │
       │                     └────────────┬───────────┘
       │                                  │
       ▼                                  ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Build Process Flow                           │
│                                                                 │
│  For each test directory:                                       │
│                                                                 │
│  1. Generate Code (NanopbProto builder)                         │
│     .proto + .options → .pb.c + .pb.h                          │
│                                                                 │
│  2. Compile (Program builder)                                   │
│     .c files + .pb.c + shared .o files → executable            │
│                                                                 │
│  3. Execute (RunTest builder)                                   │
│     Run executable → capture output                             │
│                                                                 │
│  4. Verify (Decode/Encode/Compare/Match builders)               │
│     Cross-check with protoc, pattern matching, etc.             │
└─────────────────────────────────────────────────────────────────┘
```

## Component Interaction Diagram

```
┌───────────────────────────────────────────────────────────────────────┐
│                          site_scons/                                  │
│ ┌────────────────┐  ┌──────────────────┐  ┌────────────────────────┐ │
│ │ site_init.py   │  │ site_tools/      │  │ platforms/             │ │
│ │                │  │   nanopb.py      │  │   avr/                 │ │
│ │ • Platforms    │  │                  │  │   stm32/               │ │
│ │ • Builders:    │  │ • NanopbProto    │  │   mips/                │ │
│ │   - RunTest    │  │ • NanopbProtoCpp │  │   mipsel/              │ │
│ │   - Decode     │  │ • Auto-detection │  │   riscv64/             │ │
│ │   - Encode     │  │                  │  │                        │ │
│ │   - Compare    │  │                  │  │ • Cross-compilers      │ │
│ │   - Match      │  │                  │  │ • Simulators/runners   │ │
│ └────────────────┘  └──────────────────┘  └────────────────────────┘ │
└───────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ Provides custom functionality to
                                    ▼
┌───────────────────────────────────────────────────────────────────────┐
│                           SConstruct                                  │
│                                                                       │
│  env = Environment()                                                  │
│  env.Tool("nanopb")              ← Loads site_tools/nanopb.py        │
│  add_nanopb_builders(env)        ← Calls function from site_init.py  │
│                                                                       │
│  if PLATFORM:                                                         │
│      platforms[PLATFORM](env)    ← Calls platform config function    │
│                                                                       │
│  SConscript("common/SConscript") ← Process shared resources          │
│  for each test:                                                       │
│      SConscript("test/SConscript") ← Process individual tests        │
└───────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ Triggers builds
                                    ▼
┌───────────────────────────────────────────────────────────────────────┐
│                        Test Execution                                 │
│                                                                       │
│  common/SConscript:                                                   │
│    pb_encode.o ──┐                                                    │
│    pb_decode.o ──┼─── Linked into each test                          │
│    person.pb.c ──┘                                                    │
│                                                                       │
│  test_x/SConscript:                                                   │
│    1. NanopbProto("message") → message.pb.c, message.pb.h            │
│    2. Program(encode) → build/test_x/encode                           │
│    3. RunTest(encode) → build/test_x/encode.output                    │
│    4. Decode(...) → build/test_x/encode.decoded                       │
│    5. Compare([...]) → verify correctness                             │
└───────────────────────────────────────────────────────────────────────┘
```

## Data Flow: Single Test Execution

```
┌──────────────┐
│ message.proto│
│ message.options
└──────┬───────┘
       │
       │ NanopbProto builder
       │ (via site_tools/nanopb.py)
       │
       ▼
┌──────────────────────────┐      ┌─────────────────┐
│ Generated C Code:        │      │ Test Source:    │
│ • message.pb.c           │─────▶│ • encode.c      │
│ • message.pb.h           │      │ • decode.c      │
└──────────────────────────┘      └─────────┬───────┘
                                            │
┌───────────────────────┐                   │ Program builder
│ Nanopb Core:          │                   │ (standard SCons)
│ • pb_encode.o         │───────────────────┤
│ • pb_decode.o         │                   │
│ • pb_common.o         │                   │
└───────────────────────┘                   ▼
                                   ┌─────────────────┐
                                   │ Executables:    │
                                   │ • encode        │
                                   │ • decode        │
                                   └────────┬────────┘
                                            │
                                            │ RunTest builder
                                            │ (via site_init.py)
                                            │
                     ┌──────────────────────┴──────────────────────┐
                     │                                             │
                     ▼                                             ▼
            ┌─────────────────┐                          ┌─────────────────┐
            │ encode runs:    │                          │ decode runs:    │
            │ • Writes binary │                          │ • Reads binary  │
            │   protobuf      │                          │ • Writes text   │
            │   to stdout     │                          │   to stdout     │
            └────────┬────────┘                          └────────┬────────┘
                     │                                            │
                     ▼                                            ▼
            ┌─────────────────┐                          ┌─────────────────┐
            │ encode.output   │──────────────────────────│ decode.output   │
            │ (binary data)   │  Used as input           │ (text data)     │
            └────────┬────────┘                          └────────┬────────┘
                     │                                            │
                     │ Decode builder                             │
                     │ (via site_init.py)                         │
                     │ Uses protoc                                │
                     │                                            │
                     ▼                                            │
            ┌─────────────────┐                                  │
            │ encode.decoded  │                                  │
            │ (text data from │                                  │
            │  protoc)        │                                  │
            └────────┬────────┘                                  │
                     │                                            │
                     │ Compare builder                            │
                     │ (via site_init.py)                         │
                     └────────────────┬───────────────────────────┘
                                      │
                                      ▼
                              ┌───────────────┐
                              │ Verification: │
                              │ ✓ Files match │
                              │   Test passes │
                              └───────────────┘
```

## Builder Pipeline Example

For the `basic_buffer` test:

```
SConscript Commands                    Generated Actions
────────────────────────────────────────────────────────────────────

Import("env")                          (No action - just setup)

enc = env.Program([                    gcc -c encode_buffer.c
    "encode_buffer.c",                 → encode_buffer.o
    "$COMMON/person.pb.c",             
    "$COMMON/pb_encode.o",             ld encode_buffer.o person.pb.o
    "$COMMON/pb_common.o"                 pb_encode.o pb_common.o
])                                     → build/basic_buffer/encode_buffer

dec = env.Program([                    gcc -c decode_buffer.c
    "decode_buffer.c",                 → decode_buffer.o
    "$COMMON/person.pb.c",
    "$COMMON/pb_decode.o",             ld decode_buffer.o person.pb.o
    "$COMMON/pb_common.o"                 pb_decode.o pb_common.o
])                                     → build/basic_buffer/decode_buffer

env.RunTest(enc)                       ./build/basic_buffer/encode_buffer
                                       > build/basic_buffer/encode_buffer.output

env.RunTest([dec,                      ./build/basic_buffer/decode_buffer
  "encode_buffer.output"])             < build/basic_buffer/encode_buffer.output
                                       > build/basic_buffer/decode_buffer.output

env.Decode([                           protoc --decode=Person person.proto
  "encode_buffer.output",              < build/basic_buffer/encode_buffer.output
  "$COMMON/person.proto"],             > build/basic_buffer/encode_buffer.decoded
  MESSAGE="Person")

env.Compare([                          Binary compare:
  "decode_buffer.output",              decode_buffer.output ==?
  "encode_buffer.decoded"])            encode_buffer.decoded
                                       → Pass/Fail
```

## Platform Support Architecture

```
                         ┌─────────────────┐
                         │  User Command   │
                         │                 │
                         │ scons PLATFORM=X│
                         └────────┬────────┘
                                  │
                                  ▼
                    ┌──────────────────────────┐
                    │  SConstruct checks       │
                    │  ARGUMENTS['PLATFORM']   │
                    └────────┬─────────────────┘
                             │
             ┌───────────────┼───────────────┐
             │               │               │
             ▼               ▼               ▼
      ┌───────────┐   ┌───────────┐   ┌───────────┐
      │ Native    │   │ AVR       │   │ STM32     │
      │           │   │           │   │           │
      │ Uses host │   │ Cross-    │   │ Cross-    │
      │ compiler  │   │ compile   │   │ compile   │
      │           │   │ with      │   │ with      │
      │ Direct    │   │ avr-gcc   │   │ arm-gcc   │
      │ execution │   │           │   │           │
      │           │   │ Run with  │   │ Run with  │
      │           │   │ simavr    │   │ OpenOCD   │
      └───────────┘   └───────────┘   └───────────┘
                             │
                             │ Both produce
                             ▼
                    ┌─────────────────┐
                    │ Test Results    │
                    │ • Output files  │
                    │ • Pass/Fail     │
                    └─────────────────┘
```

## Dependency Graph Example

For `alltypes` test with optional fields:

```
alltypes.proto + alltypes.options
          │
          │ (NanopbProto)
          │
          ├──→ alltypes.pb.c
          └──→ alltypes.pb.h
                    │
                    │
         ┌──────────┴──────────┐
         │                     │
         ▼                     ▼
    encode_alltypes.c    decode_alltypes.c
         │                     │
         │ (+ pb_encode.o)     │ (+ pb_decode.o)
         │                     │
         ▼                     ▼
    encode_alltypes      decode_alltypes
         │                     
         │ (RunTest ARGS=['1'])
         ▼                     
    optionals.output          
         │                     
         │ (RunTest with input)
         ├─────────────────────▶ decode_alltypes
         │                             │
         │                             ▼
         │                      optionals.decout
         │
         │ (Decode MESSAGE='AllTypes')
         ▼
    optionals.output.decoded
         │
         │ (Encode MESSAGE='AllTypes')
         ▼
    optionals.output.recoded
         │
         │ (Compare)
         ├──→ Compare with optionals.output
         │
         └──→ ✓ Pass or ✗ Fail
```

## File Organization

```
tests/
│
├── SConstruct                    Entry point
├── Makefile                      Wrapper (calls scons)
│
├── site_scons/                   Custom SCons extensions
│   ├── site_init.py             Auto-loaded by SCons
│   ├── site_tools/              Custom tools
│   │   └── nanopb.py           Proto code generation
│   └── platforms/               Cross-compilation configs
│       ├── avr/
│       ├── stm32/
│       └── ...
│
├── common/                       Shared resources
│   ├── SConscript              Build shared objects
│   ├── person.proto            Common test message
│   ├── unittestproto.proto     For unit tests
│   ├── test_helpers.h          Utility macros
│   └── malloc_wrappers.*       For malloc tests
│
├── basic_buffer/                Example test
│   ├── SConscript              Test build file
│   ├── encode_buffer.c         Encoder test
│   └── decode_buffer.c         Decoder test
│
├── alltypes/                    Comprehensive test
│   ├── SConscript
│   ├── alltypes.proto          All protobuf types
│   ├── alltypes.options        Code generation options
│   ├── encode_alltypes.c
│   └── decode_alltypes.c
│
├── ... (70+ more test dirs)
│
└── build/                       Build output (created)
    ├── common/                 Shared objects
    │   ├── pb_encode.o
    │   ├── pb_decode.o
    │   └── person.pb.c
    ├── basic_buffer/           Test outputs
    │   ├── encode_buffer
    │   ├── encode_buffer.output
    │   └── ...
    └── ...
```

## Key Concepts Summary

### 1. SCons Environment

The `env` object carries all configuration:

```python
env['CC']           # Compiler
env['CFLAGS']       # Compiler flags
env['PROTOC']       # Path to protoc
env['NANOPB']       # Nanopb root directory
env['COMMON']       # Path to common build dir
```

### 2. Builders

Functions that transform inputs to outputs:

```
Builder         Input                    Output
─────────────────────────────────────────────────────────────
NanopbProto     .proto + .options   →   .pb.c + .pb.h
Program         .c + .o files       →   executable
RunTest         executable          →   .output file
Decode          binary + .proto     →   .decoded file
Encode          text + .proto       →   .encoded file
Compare         file1 + file2       →   pass/fail
Match           output + patterns   →   pass/fail
```

### 3. Variant Directories

Source and build files are kept separate:

```
Source:  tests/basic_buffer/encode.c
Build:   tests/build/basic_buffer/encode.o
         tests/build/basic_buffer/encode
         tests/build/basic_buffer/encode.output
```

### 4. Dependency Tracking

SCons automatically tracks dependencies:

```
person.proto changes
    → person.pb.c regenerated
        → encode_buffer recompiled
            → encode_buffer.output regenerated
                → decode_buffer.output regenerated
```

### 5. Platform Abstraction

Same test code runs on multiple platforms:

```python
# Test code (platform-agnostic)
enc = env.Program(["encode.c", "msg.pb.c", ...])
env.RunTest(enc)

# Platform-specific execution:
# Native:  ./encode > output
# AVR:     simavr encode > output  
# STM32:   openocd run encode > output
```

## Extension Points

To extend the test system:

1. **Add a new builder** → Modify `site_scons/site_init.py`
2. **Add a new tool** → Create `site_scons/site_tools/mytool.py`
3. **Add a new platform** → Create `site_scons/platforms/myplatform/`
4. **Add a new test** → Create new directory with `SConscript`

## Related Documentation

- `TESTING_GUIDE.md` - Comprehensive guide with examples
- `SITE_SCONS_REFERENCE.md` - Technical reference for site_scons
- `WRITING_TESTS_QUICKSTART.md` - Quick start for writing tests

## Conclusion

The nanopb test architecture is designed around:

- **Modularity**: Tests are independent
- **Reusability**: Common code shared via `common/`
- **Extensibility**: Easy to add platforms, builders, tests
- **Automation**: Dependency tracking, incremental builds
- **Verification**: Cross-checking with protoc ensures correctness

Understanding this architecture enables you to confidently extend and enhance the test suite.
