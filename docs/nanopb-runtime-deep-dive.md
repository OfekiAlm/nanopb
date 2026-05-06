# nanopb Runtime Deep Dive

A technical guide to nanopb's runtime decoding internals, based on the actual source files in this repository.

---

## Table of Contents

1. [Runtime Architecture](#1-runtime-architecture)
2. [Decode Flow](#2-decode-flow)
3. [Core Structures](#3-core-structures)
4. [pb_common.c](#4-pb_commonc)
5. [pb_decode.c](#5-pb_decodec)
6. [Descriptor-to-Struct Mapping](#6-descriptor-to-struct-mapping)
7. [Wire Format Walkthrough](#7-wire-format-walkthrough)
8. [Callback Decoding](#8-callback-decoding)
9. [Submessages and Oneof](#9-submessages-and-oneof)
10. [Presence, Defaults, and Errors](#10-presence-defaults-and-errors)
11. [Generator Modification Safety](#11-generator-modification-safety)
12. [Debugging Checklist](#12-debugging-checklist)
13. [Mini Decoder Exercise](#13-mini-decoder-exercise)
14. [Exercises](#14-exercises)

---

## 1. Runtime Architecture

```
┌────────────────────────────────────────────────────────────┐
│                  .proto file (source of truth)             │
└──────────────────────────┬─────────────────────────────────┘
                           │  nanopb_generator.py
              ┌────────────▼────────────┐
              │   generated .pb.h       │  struct defs, init macros,
              │   generated .pb.c       │  PB_BIND(), field descriptor arrays
              └────────────┬────────────┘
                           │  compiled together with
        ┌──────────────────▼──────────────────────┐
        │              pb.h                        │  type system, macros,
        │              pb_common.c / .h            │  field iteration
        │              pb_decode.c / .h            │  wire parsing, struct fill
        └─────────────────────────────────────────┘
```

### Responsibilities

| Component | Responsibility |
|---|---|
| `nanopb_generator.py` | Reads `.proto` + options, emits `.pb.h`/`.pb.c` with C structs and field descriptor tables |
| Generated `.pb.h` | Struct definitions, `has_*` / `*_count` companion fields, `init_zero` / `init_default` macros, `*_size` constants |
| Generated `.pb.c` | `PB_BIND()` macro expansion that creates the `*_field_info[]`, `*_submsg_info[]`, and `*_msg` descriptor objects |
| `pb.h` | Type system (`pb_type_t`, `pb_msgdesc_t`, `pb_field_iter_t`), field descriptor bit-packing macros, wire-type enum, error macros |
| `pb_common.c` | Field iterator (`pb_field_iter_begin`, `pb_field_iter_next`, `pb_field_iter_find`), descriptor traversal, `pData`/`pSize` pointer computation |
| `pb_decode.c` | Stream reading, tag/varint/fixed/string decoding, struct population, required-field validation, callback dispatch |

### The Generator–Runtime Contract

The generator and runtime communicate through the **field descriptor array** embedded in `.pb.c`. Every field in a message is represented as 1, 2, 4, or 8 packed 32-bit words in a `const uint32_t` array. The words encode:

- Descriptor word count (bits 1:0 of word 0)
- Field tag number (bits 7:2 of word 0, plus extensions in higher words)
- Field type byte — an `OR` of `PB_ATYPE_*`, `PB_HTYPE_*`, `PB_LTYPE_*` (bits 15:8 of word 0)
- `data_offset` — offset of the field within the destination struct
- `data_size` — `sizeof()` of one element
- `size_offset` — signed offset from `pField` back to the `has_*` / `*_count` field
- `array_size` — max elements for repeated/fixed-count fields

If any of these values are wrong, the runtime will write into wrong memory, skip fields, or interpret bytes incorrectly. The generator must never produce values that exceed the bit widths the chosen descriptor format can hold (asserted via `PB_FIELDINFO_ASSERT_*` static asserts in `pb.h`).

---

## 2. Decode Flow

### User Entry Point

```c
MyMessage msg = MyMessage_init_zero;
pb_istream_t stream = pb_istream_from_buffer(buffer, size);
bool ok = pb_decode(&stream, MyMessage_fields, &msg);
```

`MyMessage_fields` is a `#define` that expands to `&MyMessage_msg` (a `const pb_msgdesc_t *`).

### Call Graph

```
pb_decode()                            pb_decode.c:1226
  └─ pb_decode_ex(..., flags=0)        pb_decode.c:1198
       └─ pb_decode_inner()            pb_decode.c:1027
            ├─ pb_field_iter_begin()   pb_common.c:156   build iterator, load first descriptor
            ├─ pb_message_set_to_defaults()              zero/init all fields
            │    └─ pb_field_set_to_default() per field
            └─ loop: pb_decode_tag()  pb_decode.c:278   read varint → tag + wire_type
                 ├─ pb_field_iter_find()  pb_common.c:195  find matching descriptor
                 └─ decode_field()     pb_decode.c:835
                      ├─ decode_static_field()   pb_decode.c:488   PB_ATYPE_STATIC
                      │    ├─ decode_basic_field()  pb_decode.c:417
                      │    │    ├─ pb_dec_bool()
                      │    │    ├─ pb_dec_varint()
                      │    │    ├─ pb_decode_fixed32() / pb_decode_fixed64()
                      │    │    ├─ pb_dec_bytes()
                      │    │    ├─ pb_dec_string()
                      │    │    └─ pb_dec_submessage()  → recurse pb_decode_inner()
                      │    └─ (handles OPTIONAL has_*, REPEATED count, ONEOF which_*)
                      ├─ decode_pointer_field()  pb_decode.c:645   PB_ATYPE_POINTER (malloc)
                      └─ decode_callback_field() pb_decode.c:773   PB_ATYPE_CALLBACK
```

### Key Invariant

`pb_decode_inner()` loops over wire tags until EOF. For each tag it:
1. Finds the field descriptor matching the tag.
2. Dispatches to the appropriate decode path.
3. Marks required fields seen in a bitfield.
4. On EOF verifies all required fields were present.

---

## 3. Core Structures

### `pb_istream_t`  (`pb_decode.h:28`)

```c
struct pb_istream_s {
    bool (*callback)(pb_istream_t *stream, pb_byte_t *buf, size_t count);
    void *state;         // opaque pointer; for buffer streams = current read pointer
    size_t bytes_left;   // countdown; reaches 0 at end of (sub)stream
    const char *errmsg;  // set on first error, never cleared
};
```

**Why it exists:** Decouples the decoder from the storage medium. Bytes can come from a RAM buffer, UART ring buffer, file descriptor, etc.

**Who creates it:** User code via `pb_istream_from_buffer()`, or custom callback.

**Who consumes it:** Every function in `pb_decode.c`. The stream is threaded through the entire call chain.

**Critical pointers:**
- `state` — for `buf_read`, points to the next byte to read. Advances as bytes are consumed.
- Substreams copy the parent stream struct then overwrite `bytes_left`. `pb_close_string_substream()` syncs `state` back.

**Common bugs:**
- Forgetting to check the return value of `pb_decode()` and then reading an uninitialised struct.
- Using a stream after an error (errmsg is set but stream is not reset).
- Passing wrong `msglen` to `pb_istream_from_buffer()` (too short → "end-of-stream", too long → extra bytes are garbage).

---

### `pb_msgdesc_t`  (`pb.h:350`)

```c
struct pb_msgdesc_s {
    const uint32_t *field_info;         // packed field descriptor words
    const pb_msgdesc_t * const *submsg_info; // NULL-terminated array of submessage descriptors
    const pb_byte_t *default_value;     // encoded protobuf with non-zero defaults, or NULL
    bool (*field_callback)(...);        // message-level callback (e.g. pb_default_field_callback)
    pb_size_t field_count;
    pb_size_t required_field_count;
    pb_size_t largest_tag;              // used as fast upper-bound check in pb_field_iter_find
};
```

**Why it exists:** Describes the complete schema of one message type at runtime with no dynamic allocation.

**Who creates it:** `PB_BIND()` in the generated `.pb.c`. Every message gets one `const pb_msgdesc_t MyMessage_msg`.

**Who consumes it:** `pb_field_iter_begin()`, `pb_decode_inner()`.

---

### `pb_field_iter_t`  (`pb.h:363`)

```c
struct pb_field_iter_s {
    const pb_msgdesc_t *descriptor;
    void *message;            // base pointer of the destination struct

    pb_size_t index;          // field ordinal (0..field_count-1)
    pb_size_t field_info_index; // current word offset in descriptor->field_info
    pb_size_t required_field_index;
    pb_size_t submessage_index;

    pb_size_t tag;
    pb_size_t data_size;      // sizeof(element)
    pb_size_t array_size;     // max elements (1 for scalar/optional)
    pb_type_t type;           // PB_ATYPE | PB_HTYPE | PB_LTYPE

    void *pField;  // message + data_offset  (base of array or scalar)
    void *pData;   // same as pField for static scalars; for arrays, advances per element
    void *pSize;   // &has_field, &count_field, or &which_field depending on type
    const pb_msgdesc_t *submsg_desc; // for submessage fields
};
```

**Why it exists:** Cursor over the field descriptor table. It is recomputed from the packed words by `load_descriptor_values()`.

**Who creates it:** Stack-allocated in `pb_decode_inner()` and anywhere else that iterates fields.

**Critical pointers:**
- `pField` always points into the destination struct; never NULL when `message != NULL`.
- `pData` is the write target for the value. For pointer fields (`PB_ATYPE_POINTER`), `pData = *(void**)pField` — the double indirection matters.
- `pSize` points to the `has_`/`_count`/`which_` companion field. Its type varies: `bool*` for optional, `pb_size_t*` for repeated/oneof.

**Common bugs:**
- Treating `pData` as `pField` for pointer-allocation types (they differ).
- Writing to `pData` after calling `decode_basic_field` when you should write to `pField` for pointer types.
- Forgetting that `pSize` is NULL for required fields and proto3 scalars.

---

### Field Descriptor Words

Four formats, selected by the lowest 2 bits of word 0 (0=1word, 1=2words, 2=4words, 3=8words):

```
1-word:  [2:len][6:tag][8:type][8:data_offset][4:size_offset][4:data_size]
2-word:  [2:len][6:tag][8:type][12:array_size][4:size_offset]
         [16:data_offset][12:data_size][4:tag>>6]
4-word:  [2:len][6:tag][8:type][16:array_size]
         [8:size_offset][24:tag>>6]
         [32:data_offset]
         [32:data_size]
8-word:  same as 4-word but array_size in a 5th word
```

The generator chooses the smallest format that fits all values; `PB_FIELDINFO_ASSERT_*` macros in `pb.h` enforce the fit at compile time.

---

### Generated `*_fields` / `*_msg`

```c
// In .pb.c, expanded from PB_BIND(MyMessage, MyMessage, AUTO):
const uint32_t MyMessage_field_info[] PB_PROGMEM = { /* packed words */ 0 };
const pb_msgdesc_t * const MyMessage_submsg_info[] = { /* submsg ptrs */ NULL };
const pb_msgdesc_t MyMessage_msg = {
    MyMessage_field_info,
    MyMessage_submsg_info,
    MyMessage_DEFAULT,   // NULL if no defaults
    MyMessage_CALLBACK,  // pb_default_field_callback or NULL
    <field_count>, <required_field_count>, <largest_tag>
};
// In .pb.h:
#define MyMessage_fields (&MyMessage_msg)
```

---

### `pb_callback_t`  (`pb.h:430`)

```c
struct pb_callback_s {
    union {
        bool (*decode)(pb_istream_t *stream, const pb_field_t *field, void **arg);
        bool (*encode)(pb_ostream_t *stream, const pb_field_t *field, void * const *arg);
    } funcs;
    void *arg;   // free user pointer, passed as **arg to allow mutation
};
```

**Why it exists:** For fields whose data cannot be bounded at compile time (e.g. dynamically sized strings, repeated submessages). The callback is stored _inside_ the destination struct in place of the field data.

**Who creates it:** User code, before calling `pb_decode()`. Must set `funcs.decode` before decode; the runtime never sets it.

---

### `pb_bytes_array_t`  (`pb.h:405`)

```c
struct pb_bytes_array_s {
    pb_size_t size;
    pb_byte_t bytes[1]; // flexible array member; actual array longer
};
// Helper macro for stack allocation:
#define PB_BYTES_ARRAY_T(n) struct { pb_size_t size; pb_byte_t bytes[n]; }
```

**Why it exists:** `bytes` fields need a length prefix because they are not null-terminated.

**Common bugs:**
- Allocating the struct with `sizeof(pb_bytes_array_t)` — this only gives 1 byte of payload. Use `PB_BYTES_ARRAY_T_ALLOCSIZE(n)` or `PB_BYTES_ARRAY_T(n)`.

---

## 4. `pb_common.c`

`pb_common.c` owns **field iteration** and **descriptor traversal**. It is used by both encoder and decoder. It contains no I/O.

### `pb_field_iter_begin()`  (`pb_common.c:156`)

```
Input:  iter (output), desc (pb_msgdesc_t*), message (void*)
Output: true if message has at least one field
Effect: zeroes iter, stores desc+message, calls load_descriptor_values() for field 0
```

**Pseudocode:**
```c
memset(iter, 0, sizeof(*iter));
iter->descriptor = desc;
iter->message    = message;
return load_descriptor_values(iter);
```

---

### `load_descriptor_values()` (static, `pb_common.c:8`)

Unpacks the packed 32-bit descriptor words for the current `iter->field_info_index` into the human-readable fields of `pb_field_iter_t`. Then computes `pField`, `pSize`, `pData`, and `submsg_desc`.

```
pField = (char*)message + data_offset
pSize  = (char*)pField - size_offset   (if size_offset != 0)
       = &iter->array_size             (if fixed-count repeated)
       = NULL                          (required / proto3 scalar)
pData  = *(void**)pField               (pointer fields)
       = pField                        (static fields)
submsg_desc = descriptor->submsg_info[submessage_index]
```

**Why `size_offset` is signed:** The `has_*` or `*_count` field precedes the data field in the struct layout. The offset is stored as a negative byte delta from `pField` back to the companion field.

---

### `advance_iterator()` (static, `pb_common.c:122`)

Increments `iter->index`. When wrapping past the last field, resets all counters to 0 (supports circular iteration in `pb_field_iter_find`).

Between fields it reads the previous descriptor's format word to know how many words to skip (`1 << (prev_descriptor & 3)`), and updates `required_field_index` and `submessage_index` accordingly.

---

### `pb_field_iter_next()`  (`pb_common.c:188`)

```c
advance_iterator(iter);
load_descriptor_values(iter);
return iter->index != 0;  // false when wrapped around
```

---

### `pb_field_iter_find()`  (`pb_common.c:195`)

Efficiently finds a field by tag number. Uses the `largest_tag` fast-path: if `tag > descriptor->largest_tag`, returns false immediately.

For the search it does a **cheap pre-check** — comparing only the lower 6 bits of the tag against `(fieldinfo >> 2) & 0x3F` — before calling `load_descriptor_values()` (which is more expensive). Full tag comparison happens only on candidates.

Fields are stored in **ascending tag order**, so `pb_field_iter_find()` only resets to the beginning if the target tag is less than the current tag.

---

### Destination Address Computation

```
dest struct base  ──► pField (base of field or array start)
                           │
                    size_offset (signed)
                           │
                           ▼
                       pSize  ─► has_fieldname  (bool, optional)
                                  fieldname_count (pb_size_t, repeated)
                                  which_union     (pb_size_t, oneof)
```

For a repeated field with 3 elements already decoded and `data_size = 4`:
```
pField  = &msg.scores[0]
pSize   = &msg.scores_count
pData   = (char*)pField + data_size * (*pSize)   ← set in decode_static_field
```

---

## 5. `pb_decode.c`

### Stream Layer

#### `pb_read()` (`pb_decode.c:81`)

The only function allowed to consume bytes from a stream. Checks `bytes_left`, calls `callback` (or `buf_read` in buffer-only mode), decrements `bytes_left`.

#### `pb_readbyte()` (static, `pb_decode.c:124`)

Optimised single-byte read for the hot varint loop.

#### `pb_istream_from_buffer()` (`pb_decode.c:142`)

```c
stream.callback  = &buf_read;
stream.state     = (void*)buf;   // const cast via union
stream.bytes_left = msglen;
```

`buf_read` (static) just does `memcpy` from `state` and advances the pointer.

---

### Varint Decoding

#### `pb_decode_varint32()` (`pb_decode.c:171`)

Reads bytes with the MSB continuation bit:
```
byte & 0x80  → more bytes follow
byte & 0x7F  → 7 payload bits at current bit position
```

Fast path for single-byte values. Multi-byte path accumulates bits. Validates against overflow (> 32 bits for unsigned, negative sign extension for signed).

**Pseudocode:**
```c
read first byte
if (byte & 0x80) == 0: result = byte; return
result = byte & 0x7F; bitpos = 7
do:
    read byte
    validate overflow at bitpos >= 32
    result |= (byte & 0x7F) << bitpos
    bitpos += 7
while byte & 0x80
```

#### `pb_decode_varint()` (`pb_decode.c:230`)

64-bit version. Checks overflow at `bitpos >= 63`.

#### `pb_decode_svarint()` (`pb_decode.c:1391`)

ZigZag decode: `(value >> 1) ^ -(value & 1)`. Used for `sint32` / `sint64`.

---

### Tag Decoding

#### `pb_decode_tag()` (`pb_decode.c:278`)

```c
read varint → temp
*tag       = temp >> 3
*wire_type = (pb_wire_type_t)(temp & 7)
```

Returns false with `*eof = true` on clean end-of-stream.

Wire types:
| Constant | Value | Used for |
|---|---|---|
| `PB_WT_VARINT` | 0 | int32, int64, bool, enum |
| `PB_WT_64BIT` | 1 | fixed64, double |
| `PB_WT_STRING` | 2 | string, bytes, submessages, packed arrays |
| `PB_WT_32BIT` | 5 | fixed32, float |

---

### Fixed-Width Decoding

#### `pb_decode_fixed32()` (`pb_decode.c:1405`)

Reads 4 bytes. On little-endian 8-bit platforms: direct assign. On others: explicit byte assembly.

#### `pb_decode_fixed64()` (`pb_decode.c:1428`)

Same pattern, 8 bytes.

---

### String / Length-Delimited Decoding

#### `pb_make_string_substream()` (`pb_decode.c:383`)

Creates a limited-length view of the parent stream:
```c
read varint → size
*substream = *stream           // copy all fields
substream->bytes_left = size
stream->bytes_left   -= size   // parent stream advances past the embedded content
```

#### `pb_close_string_substream()` (`pb_decode.c:398`)

Syncs `state` (read pointer) from substream back to parent. Drains any unconsumed bytes in the substream.

---

### Field Decode Dispatch

```
decode_field()
  switch PB_ATYPE(type):
    STATIC   → decode_static_field()
    POINTER  → decode_pointer_field()
    CALLBACK → decode_callback_field()
```

#### `decode_static_field()` (`pb_decode.c:488`)

```
switch PB_HTYPE(type):
  REQUIRED → decode_basic_field()
  OPTIONAL → *(bool*)pSize = true; decode_basic_field()
  REPEATED → advance pData; bounds-check array_size; decode_basic_field()
             (packed: open substream, loop until empty)
  ONEOF    → memset pData to 0 if switching variant; *(pb_size_t*)pSize = tag
             decode_basic_field()
```

#### `decode_basic_field()` (`pb_decode.c:417`)

Routes to the appropriate type decoder based on `PB_LTYPE(type)`. Enforces wire type compatibility — wrong wire type returns `"wrong wire type"` error.

---

### `pb_dec_string()` (`pb_decode.c:1572`)

1. Read varint → `size`.
2. Check `alloc_size = size + 1` fits in `field->data_size`.
3. Read `size` bytes into `pData`.
4. Write `dest[size] = 0` (null terminator).
5. If `PB_VALIDATE_UTF8`: call `pb_validate_utf8()`.

---

### `pb_dec_bytes()` (`pb_decode.c:1532`)

1. Read varint → `size`.
2. Check `PB_BYTES_ARRAY_T_ALLOCSIZE(size)` fits in `field->data_size`.
3. Write `dest->size = size`.
4. Read `size` bytes into `dest->bytes`.

---

### `pb_dec_submessage()` (`pb_decode.c:1622`)

1. `pb_make_string_substream()` — isolates submessage bytes.
2. Optionally call `LTYPE_SUBMSG_W_CB` pre-decode callback.
3. `pb_decode_inner()` — recursive decode into `field->pData`.
4. `pb_close_string_substream()`.

For repeated or pointer submessages, `PB_DECODE_NOINIT` is NOT passed (defaults must be applied). For static non-repeated, `PB_DECODE_NOINIT` is passed because the top-level decode already zeroed the struct.

---

### Callback Field Decoding

#### `decode_callback_field()` (`pb_decode.c:773`)

For `PB_WT_STRING` fields:
1. `pb_make_string_substream()` to bound the callback stream.
2. Call `field->descriptor->field_callback(&substream, NULL, field)`.
3. The callback function reads from `substream`. May be called multiple times until `substream.bytes_left == 0`.

For non-string (varint, fixed) fields:
1. Read raw bytes into a 10-byte buffer.
2. Create an in-memory substream from that buffer.
3. Call `field_callback(&substream, NULL, field)`.

---

### Unknown Field Skipping

#### `pb_skip_field()` (`pb_decode.c:318`)

Dispatches on wire type:
- `PB_WT_VARINT` → `pb_skip_varint()` (read until continuation bit clear)
- `PB_WT_64BIT` → `pb_read(stream, NULL, 8)`
- `PB_WT_STRING` → `pb_skip_string()` (read varint length, then skip that many bytes)
- `PB_WT_32BIT` → `pb_read(stream, NULL, 4)`

Passing `NULL` as buffer to `pb_read()` discards the bytes.

---

### Required-Field Validation

`pb_decode_inner()` maintains a `pb_fields_seen_t` — a bitfield of 64 bits (by default). When a required field is decoded, bit `required_field_index` is set. After stream EOF, all bits in the range `[0, required_field_count)` must be set. If any required field is absent: `"missing required field"`.

---

### Error Handling

```c
// pb.h:
#define PB_SET_ERROR(stream, msg) \
    (stream->errmsg = (stream)->errmsg ? (stream)->errmsg : (msg))
#define PB_RETURN_ERROR(stream, msg) \
    return PB_SET_ERROR(stream, msg), false
#define PB_GET_ERROR(stream) \
    ((stream)->errmsg ? (stream)->errmsg : "(none)")
```

- All decode functions return `bool`.
- `false` means an error occurred; `stream->errmsg` holds a string literal.
- The first error wins (errmsg is not overwritten).
- All callers must check return values — the `checkreturn` attribute on GCC/clang helps enforce this.

---

## 6. Descriptor-to-Struct Mapping

### Proto Definition

```proto
message Person {
  uint32 id = 1;
  string name = 2 [(nanopb).max_size = 32];
  repeated uint32 scores = 3 [(nanopb).max_count = 4];
}
```

### Generated C Struct (`.pb.h`)

```c
typedef struct _Person {
    uint32_t id;               // field 1, SINGULAR/REQUIRED (proto3: implicit)
    char name[33];             // field 2, max_size=32 → char[33] (32 + NUL)
    pb_size_t scores_count;    // companion count for field 3
    uint32_t scores[4];        // field 3, max_count=4
} Person;

#define Person_init_zero { 0, "", 0, {0, 0, 0, 0} }
```

### Memory Layout

```
Offset  Field
──────  ──────────────────────────────────────
+0      id              (4 bytes, uint32_t)
+4      name[0..32]     (33 bytes, char[33])
+(pad)  (alignment padding, if any)
+?      scores_count    (2 bytes, pb_size_t)
+?      scores[0..3]    (16 bytes, uint32_t[4])
```

_(Exact offsets are compiler/platform specific; the generator uses `offsetof()` macros to compute them.)_

### Field Descriptor Entries

For `id` (tag=1, uint32, proto3 singular):
```
type = PB_ATYPE_STATIC | PB_HTYPE_SINGULAR | PB_LTYPE_UVARINT
data_offset = offsetof(Person, id)   = 0
data_size   = sizeof(uint32_t)       = 4
size_offset = 0   (no has_* needed for proto3)
array_size  = 1
```

For `name` (tag=2, string, max_size=32):
```
type = PB_ATYPE_STATIC | PB_HTYPE_SINGULAR | PB_LTYPE_STRING
data_offset = offsetof(Person, name) = 4
data_size   = sizeof(Person.name)    = 33
size_offset = 0
array_size  = 1
```

For `scores` (tag=3, uint32 repeated):
```
type = PB_ATYPE_STATIC | PB_HTYPE_REPEATED | PB_LTYPE_UVARINT
data_offset = offsetof(Person, scores)
data_size   = sizeof(uint32_t)               = 4
size_offset = pb_delta(Person, scores, scores_count)
            = offsetof(Person, scores) - offsetof(Person, scores_count)
            = positive value (scores is laid out after scores_count)
array_size  = 4
```
`pSize = (char*)pField - size_offset` subtracts this positive delta to step back to `&scores_count`.

### How `pb_field_iter_t` Connects Descriptors to Memory

When the decoder finds tag 3 and needs to write the second score:

```
iter.pField = (char*)&msg + 40          → &msg.scores[0]
iter.pSize  = (char*)iter.pField - 2    → &msg.scores_count
iter.pData  = (char*)iter.pField
              + iter.data_size * (*iter.pSize)
            = &msg.scores[0] + 4 * 1    → &msg.scores[1]
```

After decode:
```
(*iter.pSize)++     → scores_count = 2
```

---

## 7. Wire Format Walkthrough

### Message

```proto
// Proto2 message:
message Demo {
  required uint32 id   = 1;
  required string name = 2;
}
// Encoded: id=42, name="Hi"
```

### Hex Buffer

```
0A 2A 12 02 48 69
```

### Byte-by-Byte

```
Offset  Hex   Binary      Meaning                         Runtime Function
──────  ────  ──────────  ──────────────────────────────  ─────────────────────────
0       0A    0000 1010   Tag+wire: tag=1, WT=2 (STRING)  pb_decode_tag()
                          Wait — WT=2 for a uint32? That can't be right for tag 1.
                          Let's redo with correct encoding.
```

Correct encoding for `id=42` (uint32, varint, wire type 0), `name="Hi"` (string, wire type 2):

```
08 2A 12 02 48 69
```

```
Offset  Hex   Binary      Meaning                         Runtime Function
──────  ────  ──────────  ──────────────────────────────  ─────────────────────────
0       08    0000 1000   Tag varint: value=8             pb_decode_tag()
                          tag = 8 >> 3 = 1
                          wire_type = 8 & 7 = 0 (VARINT)
1       2A    0010 1010   Varint value, MSB=0 → 1 byte    pb_decode_varint32()
                          value = 0x2A = 42
                          → msg.id = 42
2       12    0001 0010   Tag varint: value=0x12=18        pb_decode_tag()
                          tag = 18 >> 3 = 2
                          wire_type = 18 & 7 = 2 (STRING)
3       02    0000 0010   String length varint = 2         pb_make_string_substream()
                          substream.bytes_left = 2
4       48    0100 1000   'H' = 0x48                       pb_dec_string() → pb_read()
5       69    0110 1001   'i' = 0x69                       pb_read()
                          dest[2] = '\0'  (null terminator added)
                          → msg.name = "Hi"
```

### Final Struct State

```c
Demo msg = {
    .id   = 42,
    .name = "Hi"
};
```

### Runtime Path

```
pb_decode_inner()
  pb_decode_tag()        → tag=1, wire=VARINT
  pb_field_iter_find()   → iter points at 'id' descriptor
  decode_field()
    decode_static_field() [REQUIRED]
      decode_basic_field() [UVARINT]
        pb_dec_varint()
          pb_decode_varint32() → 42
          *(uint32_t*)iter.pData = 42

  pb_decode_tag()        → tag=2, wire=STRING
  pb_field_iter_find()   → iter points at 'name' descriptor
  decode_field()
    decode_static_field() [REQUIRED]
      decode_basic_field() [STRING]
        pb_dec_string()
          pb_decode_varint32() → length=2
          check alloc_size=3 <= data_size=33  ✓
          dest[2] = '\0'
          pb_read() → copies "Hi"
```

---

## 8. Callback Decoding

### Why `FT_CALLBACK` Exists

Statically-allocated fields require knowing the maximum size at compile time (`max_size`, `max_count`). When the maximum is truly unknown — arbitrary-length repeated strings, large binaries, streaming data — the `FT_CALLBACK` allocation type lets user code handle the field without pre-allocating any buffer in the struct.

### Generated Struct Change

Without callback:
```c
char name[64];          // static buffer
```

With callback (`FT_CALLBACK` in `.options` or by default for unbounded fields):
```c
pb_callback_t name;     // replaces the buffer
```

The `pb_callback_t` occupies `sizeof(pb_callback_t)` bytes in the same location in the struct.

### How `pb_callback_t` Works

The user initialises the callback **before** calling `pb_decode()`:

```c
bool my_string_callback(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    size_t len = stream->bytes_left;
    char *buf = malloc(len + 1);
    pb_read(stream, (pb_byte_t*)buf, len);
    buf[len] = '\0';
    *(char**)arg = buf;   // store pointer for later use
    return true;
}

MyMessage msg = MyMessage_init_zero;
char *result = NULL;
msg.name.funcs.decode = my_string_callback;
msg.name.arg = &result;
pb_decode(&stream, MyMessage_fields, &msg);
```

### When Callbacks Are Called

In `decode_callback_field()` (`pb_decode.c:773`), after `pb_make_string_substream()` opens a substream:

```c
do {
    prev_bytes_left = substream.bytes_left;
    field->descriptor->field_callback(&substream, NULL, field);
} while (substream.bytes_left > 0 && substream.bytes_left < prev_bytes_left);
```

The loop re-calls the callback as long as it consumed some bytes. For repeated fields, `pb_decode_inner()` calls `decode_callback_field()` once per occurrence.

### What the Callback Receives

- `stream`: a **substream** limited to exactly this field's encoded bytes. `stream->bytes_left` is the number of bytes in this field occurrence.
- `field`: the current `pb_field_iter_t`. `field->pData` points to the `pb_callback_t` itself.
- `arg`: `&field->callback.arg` — the user-set opaque pointer. Can be read and modified.

### String Callbacks

The substream contains just the raw UTF-8 bytes of the string (no length prefix — that was already consumed). Read them with `pb_read(stream, buf, stream->bytes_left)`.

### Bytes Callbacks

Same as string callbacks. The substream contains the raw bytes.

### Repeated Submessage Callbacks

For `FT_CALLBACK` on a submessage field, the substream contains the complete encoded submessage bytes. The callback can call `pb_decode()` on the substream with the submessage's descriptor to decode it:

```c
bool submsg_cb(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    Inner inner = Inner_init_zero;
    return pb_decode(stream, Inner_fields, &inner);
}
```

### Attaching Validation to Decode Callbacks

Because the callback controls decoding, it can validate the value before storing it:

```c
bool validated_string_cb(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    size_t len = stream->bytes_left;
    if (len > MAX_ALLOWED) {
        // Setting errmsg propagates the error up through pb_decode
        PB_RETURN_ERROR(stream, "string too long");
    }
    // ... store
    return true;
}
```

---

## 9. Submessages and Oneof

### Length-Delimited Submessage Decoding

Every submessage on the wire is wrapped in a `PB_WT_STRING` (length-delimited) envelope:

```
[tag | wire=2] [varint length] [submessage bytes...]
```

`pb_dec_submessage()` (`pb_decode.c:1622`) calls `pb_make_string_substream()` to carve a limited-length substream from the parent, then calls `pb_decode_inner()` recursively on the substream with the submessage's `pb_msgdesc_t`.

```
parent stream:  |..[tag][len][sub bytes]...|
                             └──substream──┘  bytes_left = len
```

After decoding, `pb_close_string_substream()` syncs the parent stream's position.

### Limited Streams

The substream guarantees the submessage decoder cannot read past its boundary. If the submessage decoder tries to read beyond `bytes_left`, `pb_read()` returns `"end-of-stream"`. Unknown fields in a submessage are skipped within the substream, not the parent.

### Recursive Decoding

Nesting depth is bounded only by stack space. Each level of recursion allocates:
- One `pb_istream_t` (substream)
- One `pb_field_iter_t`
- One `pb_fields_seen_t`

On deep-embedded systems, excessive nesting can cause stack overflow. There is no hard nesting limit enforced by nanopb.

### Generated Oneof Union

```proto
message Event {
  oneof payload {
    uint32 count = 1;
    string label = 2 [(nanopb).max_size = 32];
  }
}
```

Generated:
```c
typedef struct _Event {
    pb_size_t which_payload;    // tag of active field, or 0
    union {
        uint32_t count;
        char label[33];
    } payload;
} Event;
```

The `which_payload` field is the `pSize` target for all oneof members. Its offset is computed by `pb_delta(Event, payload.count, which_payload)`.

### `which_*` Selector

`decode_static_field()` for `PB_HTYPE_ONEOF`:
```c
if (PB_LTYPE_IS_SUBMSG(field->type) && *(pb_size_t*)field->pSize != field->tag) {
    // Different submessage variant: zero the union to clear old callbacks/pointers
    memset(field->pData, 0, field->data_size);
    // Apply defaults for the new submessage
    pb_message_set_to_defaults(&submsg_iter);
}
*(pb_size_t*)field->pSize = field->tag;  // set which_payload = tag
decode_basic_field(stream, wire_type, field);
```

### Multiple Oneof Fields in Wire Data

If the same oneof appears multiple times in the wire stream with **different tags**, each occurrence overwrites `which_payload` and the union data. The last occurrence wins. For submessage variants, the union is zeroed before the new value is decoded (to prevent stale callback pointers).

If the same oneof tag appears multiple times (same variant), the field is merged (same as a repeated optional).

---

## 10. Presence, Defaults, and Errors

### Required Fields

Proto2 `required` fields. The runtime tracks a bitfield. If any required field's bit is not set when EOF is reached, `pb_decode()` returns false with `"missing required field"`. Up to `PB_MAX_REQUIRED_FIELDS` (default 64) required fields per message.

### Optional Fields with `has_*`

Proto2 `optional` fields. The generated struct has a companion `bool has_fieldname`. When the field is present in the wire data, `decode_static_field()` sets `*(bool*)field->pSize = true`. If absent, `pb_message_set_to_defaults()` sets it to `false`.

### Proto3 Scalar Fields Without Presence

Proto3 singular scalar fields have no `has_*`. `size_offset = 0`, so `pSize = NULL`. The field is simply written when present on the wire; if absent, the zero/default value remains from `init_zero`.

### Proto3 Optional Fields

`optional` keyword in proto3 re-introduces `has_*` tracking. Treated like proto2 optional at the runtime level.

### Default Values

Non-zero defaults are encoded in `pb_msgdesc_t.default_value` as a miniature protobuf blob. `pb_message_set_to_defaults()` decodes this blob into the destination struct using `decode_field()` before applying zero initialization to everything else.

After calling a default:
```c
if (iter->pSize)
    *(bool*)iter->pSize = false;  // has_field = false even though default was written
```

This ensures the user can distinguish "field explicitly set to its default" from "field absent" only if they check `has_*`.

### `init_zero` vs `init_default`

| Macro | Effect |
|---|---|
| `MyMessage_init_zero` | All bytes zero. Safe for static allocation. No defaults applied. |
| `MyMessage_init_default` | Calls the default initializer, setting default values as per `.proto`. May contain non-zero values. |

**Important:** `pb_decode()` calls `pb_message_set_to_defaults()` internally on entry (unless `PB_DECODE_NOINIT` is set), so manually initialising with `_init_zero` is safe and recommended.

### Error Macros

```c
// Set error if not already set, then return false:
PB_RETURN_ERROR(stream, "message string");

// Just set error (no return):
PB_SET_ERROR(stream, "message string");

// Retrieve current error string:
const char *msg = PB_GET_ERROR(&stream);
```

All error strings are `const char *` pointing to string literals in ROM — no heap allocation.

---

## 11. Generator Modification Safety

When modifying `nanopb_generator.py`, the generated `.pb.c` must produce descriptor arrays that satisfy every assumption in `pb_common.c` and `pb_decode.c`.

### Critical Runtime Assumptions

| What | Assumption | What Breaks |
|---|---|---|
| `field_info[]` packing | Lowest 2 bits = descriptor word count (0→1, 1→2, 2→4, 3→8) | `advance_iterator()` skips wrong number of words → all subsequent fields are wrong |
| `data_offset` | `offsetof(struct, field)` | `pField` points into random memory |
| `size_offset` | Signed byte delta from `pField` to `has_*`/`_count`/`which_*` | `pSize` points into random memory |
| `data_size` | `sizeof(element)` | Overflow check fails or wrong bytes written |
| `array_size` | `pb_arraysize(struct, field)` | Array overflow check uses wrong bound |
| `submsg_info[]` order | Must match the order submessage fields appear in `field_info[]` | Wrong submessage descriptor used |
| `largest_tag` | Must be the maximum tag in the message | `pb_field_iter_find()` returns false for valid tags |
| `required_field_count` | Count of `PB_HTYPE_REQUIRED` fields | Required-field check bitmask is wrong |

### `PB_FIELD_32BIT`

Without this define, `pb_size_t` is `uint16_t`. This limits:
- Tag numbers to 16 bits
- `data_offset` to 16 bits (struct size limit ~65535 bytes)
- `data_size` to 16 bits
- `array_size` to 16 bits

If your struct or tag number exceeds these limits, define `PB_FIELD_32BIT`. The generator must be told via `descriptorsize: 4` or `descriptorsize: 8` option for affected messages.

### `PB_FIELDINFO_WIDTH`

The generator picks descriptor width automatically. You can force it with the `descriptorsize` field option. Choosing too small a width causes a compile-time `PB_STATIC_ASSERT` failure.

### `max_size` and `max_count`

- `max_size` sets `data_size` for `STRING` and `BYTES` fields. The decoder's `pb_dec_string()` checks `alloc_size <= field->data_size` — if you increase max_size but forget to regenerate, the old runtime check will reject valid longer strings.
- `max_count` sets `array_size`. The decoder checks `*size < field->array_size` before writing. Too small → `"array overflow"`.

### `FT_CALLBACK`

Changing a field from static to callback:
- `PB_ATYPE` changes from `PB_ATYPE_STATIC` to `PB_ATYPE_CALLBACK`.
- The struct member changes from `char buf[N]` / `uint32_t arr[N]` to `pb_callback_t`.
- `data_size` changes to `sizeof(pb_callback_t)`.
- The decoder will now call `decode_callback_field()` instead of `decode_static_field()`.
- Existing code that accesses the struct member by name will fail to compile — intentional ABI break.

### Oneof

For oneof fields:
- The union struct member name and the `which_*` selector field must match what `PB_SO_PB_HTYPE_ONEOF` computes (`pb_delta(struct, union.member, which_union)`).
- The `which_*` field must be of type `pb_size_t`.
- The decoder checks `*(pb_size_t*)pSize != tag` to decide whether to zero the union.

### Proto3 Optional

Proto3 `optional` generates a `has_fieldname` companion field. `size_offset` must be non-zero for these fields (same as proto2 optional). If you incorrectly set `size_offset = 0`, `pSize = NULL` and presence tracking is silently lost.

### Safe Place for Generated Validation Logic

The safest places to add custom validation logic in generated code:
1. **After `pb_decode()` returns** — check struct fields in user code.
2. **In a `FT_CALLBACK` decode callback** — the callback has full control and can call `PB_RETURN_ERROR`.
3. **Via `pb_validate.c`** (nanopb's own validation module) — generates validators that check constraints post-decode.

Do NOT modify `pb_decode.c` logic to add message-specific checks; that breaks the clean separation.

---

## 12. Debugging Checklist

### Best Breakpoints

| Location | Why |
|---|---|
| `pb_decode_inner()` entry | Inspect `fields` (descriptor), `dest_struct` address |
| `pb_decode_tag()` return | Inspect `tag`, `wire_type`, `eof` |
| `pb_field_iter_find()` return | Was the tag found? Check `iter.tag`, `iter.type` |
| `decode_field()` entry | Check `iter.pField`, `iter.pData`, `iter.pSize` |
| `PB_RETURN_ERROR` macro expansion | Catch any error at the moment it is set |

### Variables to Inspect

```
stream.bytes_left         — how many bytes remain
stream.errmsg             — error string if any
iter.tag                  — field tag number
iter.type                 — (hex) PB_ATYPE | PB_HTYPE | PB_LTYPE
iter.pField               — pointer into destination struct
iter.pData                — write target (may differ from pField for arrays)
iter.pSize                — pointer to has_*/count/which_
iter.data_size            — expected element size
iter.array_size           — max elements
iter.submsg_desc          — non-NULL for submessage fields
```

### Tracing Stream Position

```c
size_t pos_before = stream.bytes_left;
// ... decode call ...
size_t consumed = pos_before - stream.bytes_left;
printf("Consumed %zu bytes\n", consumed);
```

For buffer streams: `stream.state` points to the next byte in the original buffer.

### Tracing Field Iteration

```c
pb_field_iter_t iter;
if (pb_field_iter_begin(&iter, MyMessage_fields, &msg)) {
    do {
        printf("tag=%u type=0x%02x pField=%p\n",
               iter.tag, iter.type, iter.pField);
    } while (pb_field_iter_next(&iter));
}
```

### Debugging Callbacks

In your callback, check `stream->bytes_left` on entry and after reads. If `bytes_left > 0` when the callback returns, the runtime will call it again. If the callback never drains the stream, it loops forever.

### Common Decode Failures and Likely Causes

| Error Message | Likely Cause |
|---|---|
| `"end-of-stream"` | Buffer too short, or wrong `msglen` passed to `pb_istream_from_buffer` |
| `"missing required field"` | Proto2 required field absent in wire data |
| `"wrong wire type"` | Mismatch between schema and wire data (e.g., field type changed in proto) |
| `"array overflow"` | More repeated elements than `max_count` |
| `"string overflow"` | String longer than `max_size` |
| `"bytes overflow"` | Bytes field longer than `max_size` |
| `"integer too large"` | Varint value doesn't fit in declared field type |
| `"zero tag"` | Corrupt stream contains a zero tag (non-null-terminated mode) |
| `"varint overflow"` | More than 10 varint bytes in stream — likely corrupt data |
| `"invalid field descriptor"` | Submessage field has no `submsg_desc` — generator bug |
| `"failed to set defaults"` | Default value blob is malformed |
| `"callback failed"` | User callback returned false |
| `"io error"` | Custom stream callback returned false |

---

## 13. Mini Decoder Exercise

A simplified educational decoder that mimics nanopb's core decode path. Supports varints, fixed32, strings, simple submessages, and unknown field skipping.

```c
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

/* Wire types */
#define WT_VARINT  0
#define WT_64BIT   1
#define WT_STRING  2
#define WT_32BIT   5

typedef struct {
    const uint8_t *ptr;
    size_t bytes_left;
} mini_stream_t;

/* Read a single byte */
static bool read_byte(mini_stream_t *s, uint8_t *out) {
    if (s->bytes_left == 0) return false;
    *out = *s->ptr++;
    s->bytes_left--;
    return true;
}

/* Read count bytes into buf (or skip if buf == NULL) */
static bool read_bytes(mini_stream_t *s, uint8_t *buf, size_t count) {
    if (s->bytes_left < count) return false;
    if (buf) memcpy(buf, s->ptr, count);
    s->ptr += count;
    s->bytes_left -= count;
    return true;
}

/* Decode a varint, return value in *out */
static bool decode_varint(mini_stream_t *s, uint64_t *out) {
    uint64_t result = 0;
    int shift = 0;
    uint8_t byte;
    do {
        if (!read_byte(s, &byte)) return false;
        result |= (uint64_t)(byte & 0x7F) << shift;
        shift += 7;
        if (shift > 63) return false; /* overflow */
    } while (byte & 0x80);
    *out = result;
    return true;
}

/* Skip one field payload given its wire type */
static bool skip_field(mini_stream_t *s, int wire_type) {
    uint64_t len;
    switch (wire_type) {
        case WT_VARINT: {
            uint64_t dummy;
            return decode_varint(s, &dummy);
        }
        case WT_64BIT: return read_bytes(s, NULL, 8);
        case WT_32BIT: return read_bytes(s, NULL, 4);
        case WT_STRING:
            if (!decode_varint(s, &len)) return false;
            return read_bytes(s, NULL, (size_t)len);
        default: return false;
    }
}

/* ---- Message structures ---- */
typedef struct {
    uint32_t id;
    char     name[64];
    uint32_t scores[8];
    int      scores_count;
} mini_person_t;

/* ---- Field decode ---- */
static bool decode_person(mini_stream_t *s, mini_person_t *p) {
    memset(p, 0, sizeof(*p));
    while (s->bytes_left > 0) {
        uint64_t tag_wire;
        if (!decode_varint(s, &tag_wire)) break;

        uint32_t tag       = (uint32_t)(tag_wire >> 3);
        int      wire_type = (int)(tag_wire & 7);

        if (tag == 1 && wire_type == WT_VARINT) {
            /* id: uint32 */
            uint64_t val;
            if (!decode_varint(s, &val)) return false;
            p->id = (uint32_t)val;

        } else if (tag == 2 && wire_type == WT_STRING) {
            /* name: string */
            uint64_t len;
            if (!decode_varint(s, &len)) return false;
            if (len >= sizeof(p->name)) return false; /* overflow */
            if (!read_bytes(s, (uint8_t*)p->name, (size_t)len)) return false;
            p->name[len] = '\0';

        } else if (tag == 3 && wire_type == WT_VARINT) {
            /* scores: repeated uint32 (unpacked) */
            if (p->scores_count >= 8) return false;
            uint64_t val;
            if (!decode_varint(s, &val)) return false;
            p->scores[p->scores_count++] = (uint32_t)val;

        } else if (tag == 3 && wire_type == WT_STRING) {
            /* scores: packed repeated uint32 */
            uint64_t pack_len;
            if (!decode_varint(s, &pack_len)) return false;
            mini_stream_t sub = { s->ptr, (size_t)pack_len };
            s->ptr += (size_t)pack_len;
            s->bytes_left -= (size_t)pack_len;
            while (sub.bytes_left > 0) {
                if (p->scores_count >= 8) return false;
                uint64_t val;
                if (!decode_varint(&sub, &val)) return false;
                p->scores[p->scores_count++] = (uint32_t)val;
            }

        } else {
            /* Unknown field: skip */
            if (!skip_field(s, wire_type)) return false;
        }
    }
    return true;
}

int main(void) {
    /* Encode of: id=7, name="Bob", scores=[10,20] (packed) */
    /* Field 1 (tag=1, wt=0): 0x08 0x07 */
    /* Field 2 (tag=2, wt=2): 0x12 0x03 'B' 'o' 'b' */
    /* Field 3 (tag=3, wt=2): 0x1A 0x02 0x0A 0x14 */
    static const uint8_t buf[] = {
        0x08, 0x07,
        0x12, 0x03, 'B', 'o', 'b',
        0x1A, 0x02, 0x0A, 0x14
    };

    mini_stream_t s = { buf, sizeof(buf) };
    mini_person_t p;
    if (!decode_person(&s, &p)) {
        printf("Decode failed\n");
        return 1;
    }
    printf("id=%u name=%s scores=[", p.id, p.name);
    for (int i = 0; i < p.scores_count; i++)
        printf("%s%u", i ? "," : "", p.scores[i]);
    printf("]\n");
    /* Expected: id=7 name=Bob scores=[10,20] */
    return 0;
}
```

### Comparison with Real nanopb

| Aspect | Mini decoder | Real nanopb |
|---|---|---|
| Field lookup | `if (tag == N)` chain | `pb_field_iter_find()` with descriptor table |
| Type dispatch | Hardcoded per field | `PB_LTYPE` dispatch in `decode_basic_field()` |
| Presence tracking | Not done | `has_*`, `*_count`, `which_*` via `pSize` |
| Required validation | Not done | Bitfield check in `pb_decode_inner()` |
| Default values | `memset` to zero | `pb_message_set_to_defaults()` using encoded defaults |
| Callbacks | Not done | `decode_callback_field()` |
| Substreams | Manual (`sub` variable) | `pb_make_string_substream()` / `pb_close_string_substream()` |
| Error propagation | `return false` | `PB_RETURN_ERROR` + `stream->errmsg` |
| Malloc/pointer fields | Not done | `decode_pointer_field()` |

---

## 14. Exercises

### Exercise 1 — Decode One `uint32` Manually

Given the bytes `0x08 0x96 0x01`:
1. Parse the tag byte: `0x08 >> 3 = 1` (tag), `0x08 & 7 = 0` (wire type = VARINT).
2. Parse the varint `0x96 0x01`:
   - `0x96 & 0x7F = 0x16`, continuation bit set.
   - `0x01 & 0x7F = 0x01`, continuation bit clear.
   - Value = `0x16 | (0x01 << 7)` = `0x96` = **150**.
3. Field 1 is `uint32`, so `msg.field1 = 150`.

**Verify:** Run through `pb_decode_varint32()` (`pb_decode.c:171`) in a debugger with these bytes.

---

### Exercise 2 — Trace `pb_decode()` for a Simple Message

1. Create a proto2 message with one `required uint32` field.
2. Add a printf at the start of `pb_decode_inner()` printing `fields->field_count`.
3. Call `pb_decode()` and confirm the count matches your proto field count.
4. Add a printf in the `while(pb_decode_tag(...))` loop printing `tag` and `wire_type`.
5. Confirm the values match the manually parsed bytes from Exercise 1.

---

### Exercise 3 — Inspect `pb_field_iter_t`

```c
pb_field_iter_t iter;
MyMessage msg;
pb_field_iter_begin(&iter, MyMessage_fields, &msg);
do {
    printf("index=%d tag=%d type=0x%02x pField=%p pSize=%p data_size=%d array_size=%d\n",
           iter.index, iter.tag, iter.type,
           iter.pField, iter.pSize,
           (int)iter.data_size, (int)iter.array_size);
} while (pb_field_iter_next(&iter));
```

Verify each field's offset matches `offsetof(MyMessage, fieldname)`.

---

### Exercise 4 — Add Debug Prints to `pb_decode.c`

In `decode_field()` (`pb_decode.c:835`), add before the switch:

```c
printf("[decode_field] tag=%u atype=0x%x htype=0x%x ltype=0x%x pData=%p\n",
       field->tag,
       PB_ATYPE(field->type), PB_HTYPE(field->type), PB_LTYPE(field->type),
       field->pData);
```

Decode a message with multiple field types (uint32, string, repeated) and observe the dispatch path for each.

---

### Exercise 5 — Decode a Callback String Field

1. Define a proto with an unbounded string field (either omit `max_size` and force callback in `.options`, or use `FT_CALLBACK`).
2. Implement a decode callback that copies the string into a heap buffer.
3. After `pb_decode()`, verify the string contents and free the buffer.
4. Set a breakpoint inside the callback. Confirm `stream->bytes_left` equals the string length.
5. Call the callback's `stream` through `pb_read()` to observe the raw bytes.

---

### Exercise 6 — Decode a Repeated Submessage

1. Define a message `Container` with a `repeated Inner inner = 1` field.
2. Implement either a static array (set `max_count`) or a callback.
3. Encode two `Inner` instances and decode them.
4. For the static case: set a breakpoint in `decode_static_field()` at the `PB_HTYPE_REPEATED` branch and observe `*size` increment.
5. For the callback case: set a breakpoint in `decode_callback_field()` and verify the substream boundaries.

---

### Exercise 7 — Break Descriptor Metadata Intentionally

In the generated `.pb.c`, find the `field_info[]` array for a message. Manually change the `data_offset` value for one field (add 4 bytes to it).

Predict:
- Which field will be wrong?
- Will the error be a crash, silent data corruption, or a decode error?

Run the decode and verify your prediction. Restore the original value afterward.

---

### Exercise 8 — Add a Validation Hook During Decode

1. Define a message with a `uint32 age` field.
2. Instead of using a static field, make it a callback.
3. In the callback, after reading the varint, reject values > 150:

```c
bool age_callback(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    uint32_t value;
    if (!pb_decode_varint32(stream, &value)) return false;
    if (value > 150) PB_RETURN_ERROR(stream, "age out of range");
    *(uint32_t*)(*arg) = value;
    return true;
}
```

4. Confirm that `pb_decode()` returns false and `PB_GET_ERROR(&stream)` returns `"age out of range"` when age = 200 is in the wire data.
