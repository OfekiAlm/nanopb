Nanopb - Protocol Buffers for Embedded Systems
==============================================

![Latest change](https://github.com/nanopb/nanopb/actions/workflows/trigger_on_code_change.yml/badge.svg)
![Weekly build](https://github.com/nanopb/nanopb/actions/workflows/trigger_on_schedule.yml/badge.svg)

**Welcome!** Nanopb is a small code-size Protocol Buffers implementation in ANSI C. It is
especially suitable for use in microcontrollers, but fits any memory-restricted system.

* **Homepage:** https://jpa.kapsi.fi/nanopb/
* **Git repository:** https://github.com/nanopb/nanopb/
* **Documentation:** https://jpa.kapsi.fi/nanopb/docs/
* **Forum:** https://groups.google.com/forum/#!forum/nanopb
* **Stable version downloads:** https://jpa.kapsi.fi/nanopb/download/
* **Pre-release binary packages:** https://github.com/nanopb/nanopb/actions/workflows/binary_packages.yml

---

## 📚 Documentation Guide

**New to nanopb?** Start here:
- [Quick Start](#using-the-nanopb-library) - Get up and running quickly
- [examples/simple/](examples/simple/) - Working example project
- [docs/index.md](docs/index.md) - Comprehensive documentation

**Understanding the codebase?** Read the architecture docs:
- **[ARCHITECTURE.md](ARCHITECTURE.md)** - 📖 **System architecture overview** (start here!)
- [generator/README.md](generator/README.md) - Code generator guide
- [docs/generator/GENERATOR_ARCHITECTURE.md](docs/generator/GENERATOR_ARCHITECTURE.md) - Generator internals
- [docs/generator/VALIDATOR_ARCHITECTURE.md](docs/generator/VALIDATOR_ARCHITECTURE.md) - Validator internals
- [docs/runtime/README.md](docs/runtime/README.md) - Runtime library architecture

**Contributing?** See these resources:
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [docs/security.md](docs/security.md) - Security considerations
- [tests/](tests/) - Comprehensive test suite

---


Using the nanopb library
------------------------
To use the nanopb library, you need to do two things:

1. Compile your .proto files for nanopb, using `protoc`.
2. Include *pb_encode.c*, *pb_decode.c* and *pb_common.c* in your project.

The easiest way to get started is to study the project in "examples/simple".
It contains a Makefile, which should work directly under most Linux systems.
However, for any other kind of build system, see the manual steps in
README.txt in that folder.


Generating the headers
----------------------
Protocol Buffers messages are defined in a `.proto` file, which follows a standard
format that is compatible with all Protocol Buffers libraries. To use it with nanopb,
you need to generate `.pb.c` and `.pb.h` files from it:

    python generator/nanopb_generator.py myprotocol.proto  # For source checkout
    generator-bin/nanopb_generator myprotocol.proto        # For binary package

(Note: For instructions for nanopb-0.3.9.x and older, see the documentation
of that particular version [here](https://github.com/nanopb/nanopb/blob/maintenance_0.3/README.md))

The binary packages for Windows, Linux and Mac OS X should contain all necessary
dependencies, including Python, python-protobuf library and protoc. If you are
using a git checkout or a plain source distribution, you will need to install
Python separately. Once you have Python, you can install the other dependencies
with `pip install --upgrade protobuf grpcio-tools`.

You can further customize the header generation by creating an `.options` file.
See [documentation](https://jpa.kapsi.fi/nanopb/docs/concepts.html#modifying-generator-behaviour) for details.


Running the tests
-----------------
If you want to perform further development of the nanopb core, or to verify
its functionality using your compiler and platform, you'll want to run the
test suite. The build rules for the test suite are implemented using Scons,
so you need to have that installed (ex: `sudo apt install scons` or `pip install scons`).
To run the tests:

    cd tests
    scons

This will show the progress of various test cases. If the output does not
end in an error, the test cases were successful.

Note: Mac OS X by default aliases 'clang' as 'gcc', while not actually
supporting the same command line options as gcc does. To run tests on
Mac OS X, use: `scons CC=clang CXX=clang++`. Same way can be used to run
tests with different compilers on any platform.

For embedded platforms, there is currently support for running the tests
on STM32 discovery board and [simavr](https://github.com/buserror/simavr)
AVR simulator. Use `scons PLATFORM=STM32` and `scons PLATFORM=AVR` to run
these tests.


Build systems and integration
-----------------------------
Nanopb C code itself is designed to be portable and easy to build
on any platform. Often the bigger hurdle is running the generator which
takes in the `.proto` files and outputs `.pb.c` definitions.

There exist build rules for several systems:

* **Makefiles**: `extra/nanopb.mk`, see `examples/simple`
* **CMake**: `extra/FindNanopb.cmake`, see `examples/cmake`
* **SCons**: `tests/site_scons` (generator only)
* **Bazel**: `BUILD.bazel` in source root
* **Conan**: `conanfile.py` in source root
* **Meson**: `meson.build` in source root
* **PlatformIO**: https://platformio.org/lib/show/431/Nanopb
* **PyPI/pip**: https://pypi.org/project/nanopb/
* **vcpkg**: https://vcpkg.io/en/package/nanopb

And also integration to platform interfaces:

* **Arduino**: http://platformio.org/lib/show/1385/nanopb-arduino
* **Zephyr**: https://docs.zephyrproject.org/latest/services/serialization/nanopb.html


## 🎯 Key Features

- **Small Code Size** - 5-25 KB depending on features (perfect for microcontrollers)
- **Minimal RAM Usage** - ~1 KB stack plus your message structs
- **No malloc Required** - Static allocation by default (optional malloc support)
- **Highly Portable** - Pure ANSI C, works on 8-bit to 64-bit systems
- **Flexible** - Stream or buffer based I/O, multiple allocation strategies
- **Extensive Tests** - Comprehensive test coverage for reliability


## 🗺️ Project Structure

```
nanopb/
├── README.md              # You are here - Quick start guide
├── ARCHITECTURE.md        # 📖 System architecture (for developers)
│
├── pb.h, pb_*.c/h         # Runtime library (encode/decode)
├── generator/             # Code generator (Python)
│   ├── nanopb_generator.py   # Main generator
│   ├── nanopb_validator.py   # Validation support
│   └── README.md             # Generator documentation
│
├── docs/                  # Comprehensive documentation
│   ├── index.md              # Documentation index
│   ├── concepts.md           # Core concepts and usage
│   ├── reference.md          # API reference
│   ├── generator/            # Generator architecture docs
│   └── runtime/              # Runtime architecture docs
│
├── examples/              # Example projects (start here!)
│   ├── simple/               # Basic usage
│   ├── network_server/       # Network application
│   └── validation_simple/    # Validation example
│
└── tests/                 # Test suite
```


## 🚀 Quick Example

**1. Define your protocol** (`person.proto`):
```protobuf
message Person {
    int32 id = 1;
    string name = 2;
    string email = 3;
}
```

**2. Generate C code:**
```bash
python generator/nanopb_generator.py person.proto
```

**3. Use in your application:**
```c
#include <pb_encode.h>
#include <pb_decode.h>
#include "person.pb.h"

/* Encoding */
Person person = Person_init_zero;
person.id = 123;
strcpy(person.name, "John Doe");

uint8_t buffer[128];
pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
pb_encode(&stream, Person_fields, &person);

/* Decoding */
Person decoded = Person_init_zero;
pb_istream_t istream = pb_istream_from_buffer(buffer, stream.bytes_written);
pb_decode(&istream, Person_fields, &decoded);
```


## 💬 Getting Help

- **Questions?** Ask on the [forum](https://groups.google.com/forum/#!forum/nanopb)
- **Found a bug?** [Open an issue](https://github.com/nanopb/nanopb/issues)
- **Need documentation?** See [docs/index.md](docs/index.md) and [ARCHITECTURE.md](ARCHITECTURE.md)
- **Want to contribute?** Read [CONTRIBUTING.md](CONTRIBUTING.md)


## 🤝 Contributing

We welcome contributions from developers of all skill levels! Whether you're:
- 🐛 Fixing bugs
- ✨ Adding features
- 📖 Improving documentation
- 🧪 Adding tests
- 💡 Suggesting improvements

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines and [ARCHITECTURE.md](ARCHITECTURE.md) to understand the codebase.


## 📜 License

This software is released under the zlib License. See [LICENSE.txt](LICENSE.txt) for details.

