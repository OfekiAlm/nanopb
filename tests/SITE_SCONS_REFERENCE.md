# site_scons Directory Reference

This document provides an in-depth technical reference for the `site_scons/` directory, which contains custom SCons extensions for the nanopb test infrastructure.

## Table of Contents

1. [Overview](#overview)
2. [site_init.py](#site_initpy)
3. [site_tools/nanopb.py](#site_toolsnanopbpy)
4. [platforms/](#platforms)
5. [How SCons Finds site_scons](#how-scons-finds-site_scons)

---

## Overview

The `site_scons/` directory is a **special SCons directory** that provides custom functionality:

```
site_scons/
├── site_init.py           # Auto-imported before SConstruct
├── site_tools/            # SCons tools (plugins)
│   └── nanopb.py         # Tool for .proto → .pb.c conversion
└── platforms/             # Platform-specific configurations
    ├── __init__.py
    ├── avr/
    ├── stm32/
    ├── mips/
    ├── mipsel/
    └── riscv64/
```

### SCons Discovery Mechanism

When SCons runs:

1. Looks for `site_scons/` in current directory
2. Imports `site_scons/site_init.py` (if it exists)
3. Adds `site_scons/site_tools/` to tool search path
4. Processes `SConstruct`

This means **everything in `site_init.py` is available before `SConstruct` runs**.

---

## site_init.py

### Purpose

Provides:
1. **Platform registry** - Maps platform names to configuration functions
2. **Custom builders** - Test-specific build commands (RunTest, Decode, etc.)
3. **Utility functions** - Helper code for the build system

### Source Code Breakdown

#### Imports

```python
import subprocess  # For running external commands
import sys         # For Python version detection and stderr
import re          # For pattern matching in Match builder
```

#### Platform Registry

```python
from platforms.stm32.stm32 import set_stm32_platform
from platforms.avr.avr import set_avr_platform
from platforms.mips.mips import set_mips_platform
from platforms.mipsel.mipsel import set_mipsel_platform
from platforms.riscv64.riscv64 import set_riscv64_platform

platforms = {
    'STM32': set_stm32_platform,
    'AVR': set_avr_platform,
    'MIPS': set_mips_platform,
    'MIPSEL': set_mipsel_platform,
    'RISCV64': set_riscv64_platform,
}
```

**How it's used in SConstruct:**

```python
if ARGUMENTS.get('PLATFORM'):
    platform_func = platforms.get(ARGUMENTS.get('PLATFORM'))
    if not platform_func:
        print("Supported platforms: " + str(platforms.keys()))
        raise Exception("Unsupported platform: " + ARGUMENTS.get('PLATFORM'))
    else:
        platform_func(env)
```

Usage:
```bash
scons PLATFORM=AVR     # Calls set_avr_platform(env)
scons PLATFORM=STM32   # Calls set_stm32_platform(env)
```

#### Terminal Colors

```python
try:
    import colorama
    colorama.init()  # Makes ANSI color codes work on Windows
except ImportError:
    pass
```

This enables colored output (`\033[32m[ OK ]\033[0m`) on Windows terminals.

#### Python 2 UTF-8 Support

```python
if sys.version_info.major == 2:
    import codecs
    open = codecs.open  # Override built-in open() for UTF-8
```

Ensures `.proto` files with UTF-8 characters are read correctly on Python 2 (now legacy).

---

### add_nanopb_builders(env)

This function is **called from SConstruct** to register custom builders.

#### Function Signature

```python
def add_nanopb_builders(env):
    '''Add the necessary builder commands for nanopb tests.'''
```

Called in `SConstruct` as:
```python
add_nanopb_builders(env)
```

---

### Custom Builders

#### 1. RunTest Builder

**Purpose**: Execute a test program and capture its output.

**Implementation**:

```python
def run_test(target, source, env):
    # source[0] = executable to run
    # source[1] = optional input file (if len(source) > 1)
    # target[0] = output file to write stdout to
    
    if len(source) > 1:
        infile = open(str(source[1]), 'rb')  # Binary input
    else:
        infile = None
    
    if "COMMAND" in env:
        args = [env["COMMAND"]]
    else:
        args = [str(source[0])]
    
    if 'ARGS' in env:
        args.extend(env['ARGS'])  # Add command-line arguments
    
    if "TEST_RUNNER" in env:
        args = [env["TEST_RUNNER"]] + args  # Prepend simulator/runner
    
    print('Command line: ' + str(args))
    pipe = subprocess.Popen(args,
                            stdin = infile,
                            stdout = open(str(target[0]), 'w'),
                            stderr = sys.stderr)
    result = pipe.wait()
    
    # Colored output
    if result == 0:
        print('\033[32m[ OK ]\033[0m   Ran ' + args[0])
    else:
        print('\033[31m[FAIL]\033[0m   Program ' + args[0] + ' returned ' + str(result))
    return result

run_test_builder = Builder(action = run_test, suffix = '.output')
env.Append(BUILDERS = {'RunTest': run_test_builder})
```

**Usage Examples**:

```python
# Basic usage
env.RunTest(program)
# → Runs program, writes stdout to program.output

# With input file
env.RunTest([decoder, "data.bin"])
# → Runs decoder with data.bin as stdin

# With command-line arguments
env.RunTest(program, ARGS=['--verbose', '42'])
# → Runs program --verbose 42

# Custom output name
env.RunTest("custom_name.output", program)
# → Writes to custom_name.output instead of program.output

# With test runner (e.g., simulator)
# If env["TEST_RUNNER"] = "qemu-arm"
env.RunTest(program)
# → Runs: qemu-arm program
```

**Key Features**:

- Captures stdout to a file
- Passes stdin from optional input file
- Supports command-line arguments via `ARGS`
- Supports test runners (simulators) via `TEST_RUNNER`
- Returns exit code (used by SCons for dependency tracking)
- Colored success/failure messages

---

#### 2. Decode Builder

**Purpose**: Decode a binary protobuf file to text using protoc.

**Implementation**:

```python
def decode_actions(source, target, env, for_signature):
    esc = env['ESCAPE']
    dirs = ' '.join(['-I' + esc(env.GetBuildPath(d)) for d in env['PROTOCPATH']])
    return '$PROTOC $PROTOCFLAGS %s --decode=%s %s <%s >%s' % (
        dirs, env['MESSAGE'], esc(str(source[1])), esc(str(source[0])), esc(str(target[0])))

decode_builder = Builder(generator = decode_actions, suffix = '.decoded')
env.Append(BUILDERS = {'Decode': decode_builder})
```

**Usage**:

```python
env.Decode(target, [binary_file, proto_file], MESSAGE='MessageType')
```

**Example**:

```python
env.Decode("output.decoded", ["output.bin", "person.proto"], MESSAGE='Person')
```

**Generated Command**:

```bash
protoc -I. -I../generator/proto --decode=Person person.proto <output.bin >output.decoded
```

**Explanation**:

- `$PROTOC`: Path to protoc compiler
- `$PROTOCFLAGS`: Plugin flags (usually empty when using bundled protoc)
- `-I` flags: Include paths from `env['PROTOCPATH']`
- `--decode=Person`: Decode as Person message type
- `person.proto`: Schema definition
- `<output.bin`: Read binary input from file
- `>output.decoded`: Write text output to file

---

#### 3. Encode Builder

**Purpose**: Encode a text protobuf file to binary using protoc.

**Implementation**:

```python
def encode_actions(source, target, env, for_signature):
    esc = env['ESCAPE']
    dirs = ' '.join(['-I' + esc(env.GetBuildPath(d)) for d in env['PROTOCPATH']])
    return '$PROTOC $PROTOCFLAGS %s --encode=%s %s <%s >%s' % (
        dirs, env['MESSAGE'], esc(str(source[1])), esc(str(source[0])), esc(str(target[0])))

encode_builder = Builder(generator = encode_actions, suffix = '.encoded')
env.Append(BUILDERS = {'Encode': encode_builder})
```

**Usage**:

```python
env.Encode(target, [text_file, proto_file], MESSAGE='MessageType')
```

**Example**:

```python
env.Encode("output.encoded", ["output.txt", "person.proto"], MESSAGE='Person')
```

**Use Case**: Re-encode data decoded by protoc to verify byte-for-byte compatibility.

---

#### 4. Compare Builder

**Purpose**: Assert that two files are byte-for-byte identical.

**Implementation**:

```python
def compare_files(target, source, env):
    data1 = open(str(source[0]), 'rb').read()
    data2 = open(str(source[1]), 'rb').read()
    if data1 == data2:
        print('\033[32m[ OK ]\033[0m   Files equal: ' + str(source[0]) + ' and ' + str(source[1]))
        return 0
    else:
        print('\033[31m[FAIL]\033[0m   Files differ: ' + str(source[0]) + ' and ' + str(source[1]))
        return 1

compare_builder = Builder(action = compare_files, suffix = '.equal')
env.Append(BUILDERS = {'Compare': compare_builder})
```

**Usage**:

```python
env.Compare([file1, file2])
```

**Example**:

```python
# Verify nanopb decoder output matches protoc output
env.Compare(["decode_buffer.output", "encode_buffer.decoded"])
```

**Key Features**:

- Binary comparison (works with text and binary files)
- Returns 0 for success, 1 for failure (SCons interprets this)
- Colored output

---

#### 5. Match Builder

**Purpose**: Check that regex patterns exist (or don't exist) in a file.

**Implementation**:

```python
def match_files(target, source, env):
    data = open(str(source[0]), 'r', encoding = 'utf-8').read()
    patterns = open(str(source[1]), 'r', encoding = 'utf-8')
    for pattern in patterns:
        if pattern.strip():
            invert = False
            if pattern.startswith('! '):
                invert = True
                pattern = pattern[2:]
            
            status = re.search(pattern.strip(), data, re.MULTILINE)
            
            if not status and not invert:
                print('\033[31m[FAIL]\033[0m   Pattern not found in ' + str(source[0]) + ': ' + pattern)
                return 1
            elif status and invert:
                print('\033[31m[FAIL]\033[0m   Pattern should not exist, but does in ' + str(source[0]) + ': ' + pattern)
                return 1
    else:
        print('\033[32m[ OK ]\033[0m   All patterns found in ' + str(source[0]))
        return 0

match_builder = Builder(action = match_files, suffix = '.matched')
env.Append(BUILDERS = {'Match': match_builder})
```

**Usage**:

```python
env.Match([output_file, patterns_file])
```

**Example**:

`expected_output.txt` (patterns file):
```
^Successfully encoded
bytes_written: \d+
! ERROR
! FAILED
```

```python
env.Match(["program.output", "expected_output.txt"])
```

**Pattern Syntax**:

- Regular expressions (Python `re` module)
- One pattern per line
- Lines starting with `! ` are **inverted** (must NOT match)
- Empty lines are ignored
- `re.MULTILINE` mode: `^` and `$` match line boundaries

**Key Features**:

- Flexible pattern matching for non-deterministic output
- Inverted patterns to ensure errors don't occur
- UTF-8 support

---

## site_tools/nanopb.py

### Purpose

An SCons **tool** that provides the `NanopbProto` builder for converting `.proto` files to `.pb.c`/`.pb.h` files.

### Tool Structure

```python
def generate(env):
    '''Add Builder for nanopb protos.'''
    # Called when tool is loaded: env.Tool("nanopb")

def exists(env):
    '''Check if tool can be used.'''
    # Returns True if protoc and generator are available
```

### How Tools Work in SCons

Tools are loaded with:

```python
env.Tool("nanopb")
```

This calls `generate(env)` which registers builders and sets environment variables.

In SConstruct:
```python
env.Tool("nanopb")  # Loads site_scons/site_tools/nanopb.py
```

---

### Detection Functions

#### _detect_nanopb(env)

Finds the nanopb root directory.

```python
def _detect_nanopb(env):
    if 'NANOPB' in env:
        return env['NANOPB']  # User-provided path
    
    # Assume we're in tests/site_scons/site_tools
    p = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..'))
    if os.path.isdir(p) and os.path.isfile(os.path.join(p, 'pb.h')):
        return p
    
    raise SCons.Errors.StopError(NanopbWarning, "Could not find the nanopb root directory")
```

**Logic**:
1. Check if user set `NANOPB` environment variable
2. Otherwise, go up 3 directories from `site_scons/site_tools/nanopb.py`
3. Verify `pb.h` exists there
4. Raise error if not found

**Result**: `env['NANOPB']` = `/path/to/nanopb`

---

#### _detect_python(env)

Finds Python executable.

```python
def _detect_python(env):
    if 'PYTHON' in env:
        return env['PYTHON']
    
    p = env.WhereIs('python3')
    if p:
        return env['ESCAPE'](p)
    
    p = env.WhereIs('py.exe')
    if p:
        return env['ESCAPE'](p) + " -3"
    
    return env['ESCAPE'](sys.executable)
```

**Priority**:
1. User-provided `PYTHON`
2. `python3` from PATH
3. `py.exe -3` (Windows Python launcher)
4. Current Python interpreter (`sys.executable`)

---

#### _detect_protoc(env)

Finds protoc compiler.

```python
def _detect_protoc(env):
    if 'PROTOC' in env:
        return env['PROTOC']
    
    n = _detect_nanopb(env)
    p1 = os.path.join(n, 'generator-bin', 'protoc' + env['PROGSUFFIX'])
    if os.path.exists(p1):
        return env['ESCAPE'](p1)  # Binary package
    
    p = os.path.join(n, 'generator', 'protoc')
    if os.path.exists(p):
        # grpcio-tools wrapper
        if env['PLATFORM'] == 'win32':
            return env['ESCAPE'](p + '.bat')
        else:
            return env['ESCAPE'](p)
    
    p = env.WhereIs('protoc')
    if p:
        return env['ESCAPE'](p)  # System protoc
    
    raise SCons.Errors.StopError(NanopbWarning, "Could not find the protoc compiler")
```

**Search Order**:
1. User-provided `PROTOC`
2. Bundled protoc in `generator-bin/` (binary package)
3. grpcio-tools wrapper in `generator/protoc`
4. System protoc from PATH
5. Error if not found

---

#### _detect_protocflags(env)

Determines flags for protoc.

```python
def _detect_protocflags(env):
    if 'PROTOCFLAGS' in env:
        return env['PROTOCFLAGS']
    
    p = _detect_protoc(env)
    n = _detect_nanopb(env)
    p1 = os.path.join(n, 'generator', 'protoc' + env['PROGSUFFIX'])
    p2 = os.path.join(n, 'generator-bin', 'protoc' + env['PROGSUFFIX'])
    
    if p in [env['ESCAPE'](p1), env['ESCAPE'](p2)]:
        return ''  # Bundled protoc doesn't need plugin flags
    
    # System protoc needs plugin path
    e = env['ESCAPE']
    if env['PLATFORM'] == 'win32':
        return e('--plugin=protoc-gen-nanopb=' + os.path.join(n, 'generator', 'protoc-gen-nanopb.bat'))
    else:
        return e('--plugin=protoc-gen-nanopb=' + os.path.join(n, 'generator', 'protoc-gen-nanopb'))
```

**Logic**:
- Bundled protoc: No flags needed (plugin is built-in)
- System protoc: Add `--plugin=protoc-gen-nanopb=...` flag

---

#### _detect_nanopb_generator(env)

Finds nanopb generator command.

```python
def _detect_nanopb_generator(env):
    generator_cmd = os.path.join(env['NANOPB'], 'generator-bin', 'nanopb_generator' + env['PROGSUFFIX'])
    if os.path.exists(generator_cmd):
        return env['ESCAPE'](generator_cmd)  # Binary package
    else:
        return env['PYTHON'] + " " + env['ESCAPE'](os.path.join(env['NANOPB'], 'generator', 'nanopb_generator.py'))
```

**Result**:
- Binary package: `nanopb_generator` or `nanopb_generator.exe`
- Source package: `python3 nanopb_generator.py`

---

### NanopbProto Builder

#### Action Generator

```python
def _nanopb_proto_actions(source, target, env, for_signature):
    esc = env['ESCAPE']
    
    # Make protoc build inside the SConscript directory
    prefix = os.path.dirname(str(source[-1]))
    srcfile = esc(os.path.relpath(str(source[0]), prefix))
    
    # Build include directories
    include_dirs = '-I.'
    for d in env['PROTOCPATH']:
        d = env.GetBuildPath(d)
        if not os.path.isabs(d):
            d = os.path.relpath(d, prefix)
        include_dirs += ' -I' + esc(d)
    
    # Handle .pb.cpp vs .pb.c
    source_extension = os.path.splitext(str(target[0]))[1]
    header_extension = '.h' + source_extension[2:]  # .hpp for .cpp
    
    nanopb_flags = env['NANOPBFLAGS']
    if nanopb_flags:
        nanopb_flags = '--source-extension=%s,--header-extension=%s,%s:.' % (
            source_extension, header_extension, nanopb_flags)
    else:
        nanopb_flags = '--source-extension=%s,--header-extension=%s:.' % (
            source_extension, header_extension)
    
    return SCons.Action.CommandAction(
        '$PROTOC $PROTOCFLAGS %s "--nanopb_out=%s" %s' % (
            include_dirs, nanopb_flags, srcfile),
        chdir = prefix)
```

**Key Points**:

1. **Working Directory**: Runs in the directory containing `SConscript`
2. **Include Paths**: Adds `-I` for each directory in `PROTOCPATH`
3. **Extension Handling**: Supports both `.pb.c` and `.pb.cpp` output
4. **Flags**: Passes through `NANOPBFLAGS` for customization

**Example Command**:

```bash
protoc -I. -I../generator/proto --nanopb_out=--source-extension=.pb.c,--header-extension=.pb.h:. person.proto
```

---

#### Emitter

```python
def _nanopb_proto_emitter(target, source, env):
    basename = os.path.splitext(str(source[0]))[0]
    source_extension = os.path.splitext(str(target[0]))[1]
    header_extension = '.h' + source_extension[2:]
    
    # Add header file to targets
    target.append(basename + '.pb' + header_extension)
    
    # Add SConscript to sources (for working directory)
    source.append(File("SConscript"))
    
    # Add .options file if it exists
    if os.path.exists(basename + '.options'):
        source.append(basename + '.options')
    
    return target, source
```

**What the Emitter Does**:

1. **Determines Output Files**:
   - Input: `person.proto`
   - Outputs: `person.pb.c`, `person.pb.h`

2. **Adds SConscript as Source**:
   - Hack to determine working directory in action function

3. **Detects Options Files**:
   - If `person.options` exists, add as dependency
   - Ensures rebuild when options change

---

### Builder Registration

```python
_nanopb_proto_builder = SCons.Builder.Builder(
    generator = _nanopb_proto_actions,
    suffix = '.pb.c',
    src_suffix = '.proto',
    emitter = _nanopb_proto_emitter)

_nanopb_proto_cpp_builder = SCons.Builder.Builder(
    generator = _nanopb_proto_actions,
    suffix = '.pb.cpp',
    src_suffix = '.proto',
    emitter = _nanopb_proto_emitter)

def generate(env):
    env['NANOPB'] = _detect_nanopb(env)
    env['PROTOC'] = _detect_protoc(env)
    env['PROTOCFLAGS'] = _detect_protocflags(env)
    env['PYTHON'] = _detect_python(env)
    env['NANOPB_GENERATOR'] = _detect_nanopb_generator(env)
    env.SetDefault(NANOPBFLAGS = '')
    env.SetDefault(PROTOCPATH = [".", os.path.join(env['NANOPB'], 'generator', 'proto')])
    env.SetDefault(NANOPB_PROTO_CMD = '$PROTOC $PROTOCFLAGS --nanopb_out=$NANOPBFLAGS:. $SOURCES')
    
    env['BUILDERS']['NanopbProto'] = _nanopb_proto_builder
    env['BUILDERS']['NanopbProtoCpp'] = _nanopb_proto_cpp_builder
```

**Usage**:

```python
# C output
env.NanopbProto("message")
# Generates message.pb.c, message.pb.h

# C++ output
env.NanopbProtoCpp("message")
# Generates message.pb.cpp, message.pb.hpp

# With options file
env.NanopbProto(["message", "message.options"])
# Uses options file for code generation
```

---

## platforms/

Each platform subdirectory provides cross-compilation support.

### Directory Structure

```
platforms/
├── __init__.py              # Empty (makes it a Python package)
├── avr/
│   ├── __init__.py         # Empty
│   ├── avr.py              # Platform configuration function
│   ├── avr_io.c            # I/O implementation for simulator
│   └── run_test.c          # Native test runner using simavr
├── stm32/
│   ├── __init__.py
│   ├── stm32.py            # Platform configuration
│   ├── run_test.sh         # Test runner script
│   ├── vectors.c           # ARM interrupt vectors
│   └── stm32_ram.ld        # Linker script for RAM execution
└── ... (other platforms)
```

---

### Platform Configuration Pattern

Each platform provides a `set_X_platform(env)` function:

```python
def set_X_platform(env):
    # 1. Set cross-compiler
    env.Replace(CC = "platform-gcc")
    env.Replace(CXX = "platform-g++")
    
    # 2. Set compiler flags
    env.Append(CFLAGS = "-march=...")
    env.Append(LINKFLAGS = "-T linker_script.ld")
    
    # 3. Set test runner (simulator or hardware)
    env.Replace(TEST_RUNNER = "qemu-platform")
    
    # 4. Mark as embedded
    env.Replace(EMBEDDED = "PLATFORM_NAME")
    
    # 5. Platform-specific defines
    env.Append(CPPDEFINES = {'BUFFER_SIZE': 1024})
```

---

### Example: AVR Platform (Detailed)

```python
def set_avr_platform(env):
    # Build native test runner
    native = env.Clone()
    native.Append(LIBS = ["simavr", "libelf"], CFLAGS = "-Wall -Werror -g")
    runner = native.Program("build/run_test", "site_scons/platforms/avr/run_test.c")
    
    # Configure for AVR cross-compilation
    env.Replace(EMBEDDED = "AVR")
    env.Replace(CC = "avr-gcc", CXX = "avr-g++")
    env.Replace(TEST_RUNNER = "build/run_test")
    env.Append(CFLAGS = "-mmcu=atmega1284 -Dmain=app_main -Os -g -Wall")
    env.Append(CXXFLAGS = "-mmcu=atmega1284 -Dmain=app_main -Os -Wno-type-limits")
    env.Append(CPPDEFINES = {
        'PB_CONVERT_DOUBLE_FLOAT': 1,  # No hardware double support
        'UNITTESTS_SHORT_MSGS': 1,     # Less memory available
        '__ASSERT_USE_STDERR': 1,
        'MAX_ALLOC_BYTES': 32768,
        'FUZZTEST_BUFSIZE': 2048
    })
    env.Append(LINKFLAGS = "-mmcu=atmega1284")
    env.Append(LINKFLAGS = "-Wl,-Map,build/avr.map")
    
    # Build I/O helper library
    avr_io = env.Object("build/avr_io.o", "site_scons/platforms/avr/avr_io.c")
    env.Append(LIBS = avr_io)
    env.Depends(avr_io, runner)  # Ensure runner is built
```

**What This Does**:

1. **Native Runner**: Compiles `run_test.c` for x86/x64 (uses simavr library)
2. **Cross-Compiler**: Switches to `avr-gcc` for test programs
3. **MCU Selection**: Targets ATmega1284 with 128KB flash
4. **Main Rename**: `-Dmain=app_main` (simavr expects `app_main()`)
5. **Memory Limits**: Smaller buffers for embedded constraints
6. **I/O Layer**: Custom I/O for simulator communication

**Test Execution Flow**:

```
1. Test is compiled with avr-gcc → test.elf
2. RunTest calls: build/run_test test.elf
3. run_test.c loads test.elf into simavr
4. simavr executes AVR code
5. avr_io.c provides printf/scanf over simulated serial port
6. Output is captured and written to test.output
```

---

### Example: STM32 Platform

```python
def set_stm32_platform(env):
    env.Replace(EMBEDDED = "STM32")
    env.Replace(CC = "arm-none-eabi-gcc", CXX = "arm-none-eabi-g++")
    env.Replace(TEST_RUNNER = "site_scons/platforms/stm32/run_test.sh")
    env.Append(CPPDEFINES = {'FUZZTEST_BUFSIZE': 4096})
    env.Append(CFLAGS = "-mcpu=cortex-m3 -mthumb -Os")
    env.Append(CXXFLAGS = "-mcpu=cortex-m3 -mthumb -Os")
    env.Append(LINKFLAGS = "-mcpu=cortex-m3 -mthumb")
    env.Append(LINKFLAGS = "site_scons/platforms/stm32/vectors.c")
    env.Append(LINKFLAGS = "--specs=rdimon.specs")  # Semihosting
    env.Append(LINKFLAGS = "-Tsite_scons/platforms/stm32/stm32_ram.ld")
```

**Key Differences from AVR**:

1. **ARM Cortex-M3**: Different architecture
2. **Semihosting**: `--specs=rdimon.specs` enables printf over debugger
3. **RAM Execution**: Custom linker script runs code from RAM (faster, avoids flash wear)
4. **Shell Runner**: `run_test.sh` uses OpenOCD or st-link to flash and run

---

## How SCons Finds site_scons

### Discovery Process

1. **Current Directory**: SCons looks for `site_scons/` in the directory where `scons` is run

2. **site_init.py**: If found, it's imported **before** `SConstruct`

3. **site_tools/**: Added to tool search path

4. **Order of Execution**:
   ```
   site_scons/site_init.py (imported)
       ↓
   SConstruct (main build file)
       ↓
   env.Tool("nanopb") (loads site_scons/site_tools/nanopb.py)
       ↓
   SConscript files
   ```

### Why This Matters

- **Global Setup**: `site_init.py` can set up state before any build files run
- **Custom Tools**: Tools are automatically available without explicit paths
- **Portability**: No hardcoded paths; SCons finds everything automatically

---

## Summary

### Key Takeaways

1. **site_init.py**:
   - Auto-imported by SCons
   - Provides `add_nanopb_builders()` for custom builders
   - Registers platform configurations

2. **site_tools/nanopb.py**:
   - SCons tool for `.proto` → `.pb.c` conversion
   - Automatically detects protoc, Python, nanopb root
   - Handles `.options` files

3. **platforms/**:
   - Cross-compilation configurations
   - Each platform has a `set_X_platform(env)` function
   - Supports simulators and hardware

4. **Custom Builders**:
   - `RunTest`: Execute and capture output
   - `Decode`: Use protoc to decode binary
   - `Encode`: Use protoc to encode binary
   - `Compare`: Assert file equality
   - `Match`: Pattern matching

### Extension Points

To add functionality:

1. **New Builder**: Add to `add_nanopb_builders()` in `site_init.py`
2. **New Tool**: Create `site_tools/mytool.py` with `generate()` and `exists()`
3. **New Platform**: Create `platforms/myplatform/myplatform.py` with `set_myplatform_platform()`

---

This reference covers all technical details of the `site_scons/` directory. For practical examples and usage patterns, see `TESTING_GUIDE.md`.
