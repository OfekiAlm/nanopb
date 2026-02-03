# Nanopb Clarity Refactor Roadmap

**Date:** 2026-02-03  
**Scope:** Clarity-first improvements without behavior changes  
**Constraints:** No API breaks, no wire format changes, no performance regressions

---

## 1. Repo Map

### Core Modules & Responsibilities

| Module | File(s) | Responsibilities | Lines of Code |
|--------|---------|------------------|---------------|
| **Encoding** | pb_encode.c/h | Varint encoding, fixed-size encoding, stream writing | ~900 LOC |
| **Decoding** | pb_decode.c/h | Varint decoding, wire type parsing, stream reading | ~1100 LOC |
| **Common** | pb_common.c/h | Field iteration, descriptor parsing, UTF-8 validation | ~400 LOC |
| **Validation** | pb_validate.c/h | Declarative constraints, custom options validation | ~600 LOC |
| **Core Types** | pb.h | Type definitions, compilation options, macros | ~800 LOC |

### Hot Code Paths (Performance-Critical)
1. **Varint encoding/decoding** - Used for all integer types, tags, lengths
2. **Field iteration** - Circular search for tag matching  
3. **Descriptor loading** - Parses compact binary descriptor format

### Fragile Protocol-Correctness Paths
1. **Wire type validation** - Must reject invalid wire types per protobuf spec
2. **Proto3 default value handling** - Omitting default values per proto3 semantics
3. **Extension field handling** - Complex dynamic field registration

---

## 2. Top Clarity Issues (Ranked)


### Issue #1: Magic Numbers & Bit Shifting ⚠️ **HIGH SEVERITY**
**Files:** `pb_common.c`, `pb_encode.c`, `pb_decode.c`

**Problem:** Extensive bit-packing with unexplained hex constants (0xFF, 0x7F, 0x80, 0x0F, 0x3F).
Example: `iter->type = (pb_type_t)((word0 >> 8) & 0xFF);` - what bits encode the type?

**Impact:** Code is impossible to audit for correctness. New contributors can't understand descriptor encoding.

**Fix:** Define named constants:
```c
#define DESCRIPTOR_TYPE_SHIFT      8
#define DESCRIPTOR_TYPE_MASK       0xFF
iter->type = (pb_type_t)((word0 >> DESCRIPTOR_TYPE_SHIFT) & DESCRIPTOR_TYPE_MASK);
```

---

### Issue #2: Deep Nested Conditionals ⚠️ **HIGH SEVERITY**
**Files:** `pb_encode.c` (proto3 default checking), `pb_decode.c` (type dispatch)

**Problem:** 10+ levels of nested if-else. Hard to trace logic flow.

**Impact:** Cognitive overload, difficult to test individual branches.

**Fix:** Use early returns and extract type-specific helpers.

---

### Issue #3: Code Duplication ⚠️ **MEDIUM-HIGH SEVERITY**
**Files:** `pb_encode.c` and `pb_decode.c`

**Problem:** Parallel encode/decode functions with mirrored logic.

**Impact:** Changes must be synchronized. Inconsistencies accumulate.

**Fix:** Create unified dispatch tables and shared validation helpers.

---

### Issue #4: Confusing Naming 🟡 **MEDIUM SEVERITY**
**Files:** `pb_common.c`

**Problem:** Variables like `word0`, `word1`, `pField`, `pData` are cryptic.

**Impact:** Requires reverse-engineering to understand.

**Fix:** Use descriptive names like `descriptor_word_0`, `field_ptr`.

---

### Issue #5: Macros → Inline Functions 🟡 **MEDIUM SEVERITY**  
**Files:** `pb.h`

**Problem:** `PB_ATYPE()`, `PB_HTYPE()`, `PB_LTYPE()` should be inline functions for type safety.

**Fix:**
```c
static inline pb_type_t pb_atype(pb_type_t x) { 
    return x & PB_ATYPE_MASK; 
}
```

---

### Issue #6-10: See full analysis for remaining issues
- Missing comments on complex logic
- Long functions with multiple responsibilities  
- Inconsistent error handling patterns
- Duplicate comment blocks
- Inconsistent naming conventions


---

## 3. Refactor Roadmap (Commit Series)

### Phase 1: Documentation & Named Constants (Low Risk, High ROI)

#### Commit 1.1: Add descriptor format documentation
- **Files:** `pb_common.c`
- **Scope:** Add comments explaining 1/2/4/8-word descriptor formats
- **Risk:** None (documentation only)
- **Verification:** Visual inspection

#### Commit 1.2: Define descriptor bit layout constants  
- **Files:** `pb_common.c`, `pb_common.h`
- **Scope:** Replace magic numbers with `DESCRIPTOR_TYPE_SHIFT`, etc.
- **Risk:** Low (symbolic names only)
- **Verification:** Compile with `-Werror`, run `scons` in tests/

#### Commit 1.3: Define varint bit manipulation constants
- **Files:** `pb_encode.c`, `pb_decode.c`
- **Scope:** Replace `0x7F`, `0x80` with `VARINT_DATA_MASK`, `VARINT_CONTINUATION`
- **Risk:** Low  
- **Verification:** Run varint-heavy tests (alltypes, intsizes)

#### Commit 1.4: Document field iteration algorithm
- **Files:** `pb_common.c`
- **Scope:** Add comments to `pb_field_iter_find()`, `pb_field_iter_next()`
- **Risk:** None
- **Verification:** Visual review

---

### Phase 2: Macro to Static Inline (Low Risk, Medium ROI)

#### Commit 2.1: Convert type extraction macros
- **Files:** `pb.h`, `pb_common.h`
- **Scope:** Replace `PB_ATYPE`, `PB_HTYPE`, `PB_LTYPE` with inline functions
- **Risk:** Low (requires updating all callsites)
- **Verification:** Multi-compiler test, check code size, run full suite

---

### Phase 3: Extract Helper Functions (Medium Risk, High ROI)

#### Commit 3.1: Extract wire type validation
- **Files:** `pb_decode.c`
- **Scope:** Create `is_valid_wire_type_for_ltype()`
- **Risk:** Medium (logic consolidation)
- **Verification:** decode_unittests, test invalid wire types

#### Commit 3.2: Extract descriptor bit extraction
- **Files:** `pb_common.c`
- **Scope:** Create `extract_field_type()`, `extract_tag_number()`
- **Risk:** Medium (refactoring critical path)
- **Verification:** common_unittests, test all descriptor formats

---

## 4. Key Refactor Examples

### Example 1: Descriptor Bit Extraction

**Before:**
```c
word0 = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index]);
iter->type = (pb_type_t)((word0 >> 8) & 0xFF);
iter->tag = (pb_size_t)((word0 >> 2) & 0x3F);
```

**After:**
```c
descriptor_word_0 = PB_PROGMEM_READU32(iter->descriptor->field_info[iter->field_info_index]);
iter->type = (pb_type_t)((descriptor_word_0 >> DESCRIPTOR_TYPE_SHIFT) & DESCRIPTOR_TYPE_MASK);
iter->tag = (pb_size_t)((descriptor_word_0 >> DESCRIPTOR_TAG_SHIFT) & DESCRIPTOR_TAG_MASK_1WORD);
```

---

### Example 2: Macro to Inline

**Before:**
```c
#define PB_ATYPE(x) ((x) & PB_ATYPE_MASK)
```

**After:**
```c
static inline pb_type_t pb_atype(pb_type_t type) {
    return type & PB_ATYPE_MASK;
}
```

---

### Example 3: Reduce Nesting

**Before:** Deep nested if-else in proto3 default checking

**After:** Extract helpers with early returns:
```c
static bool is_default_bool(const pb_field_iter_t *field) {
    return *(bool*)field->pData == false;
}

bool pb_check_proto3_default_value(const pb_field_iter_t *field) {
    if (PB_HTYPE(field->type) != PB_HTYPE_OPTIONAL) {
        return false;
    }
    
    pb_type_t ltype = pb_ltype(field->type);
    if (ltype == PB_LTYPE_BOOL) {
        return is_default_bool(field);
    }
    // ...
}
```

---

## 5. Verification Plan

### Test Strategy
1. **Existing test suite:** `cd tests && scons` (50+ test suites)
2. **Golden vectors:** Capture encode outputs before/after, compare byte-for-byte
3. **Cross-platform:** Test on Linux (gcc, clang), AVR (simavr), STM32
4. **Code size:** Monitor with `size *.o`, accept ±5%
5. **Performance:** Benchmark with fuzztest, accept ±5%
6. **Static analysis:** `gcc -Wall -Wextra -Werror`
7. **Memory safety:** Valgrind on all tests

### Per-Commit Verification
- **Documentation:** Visual review
- **Named constants:** Compile + test suite
- **Macro to inline:** Multi-platform + code size + tests
- **Extract helpers:** Unit tests + golden vectors + full suite

---

## 6. Risks & Mitigations

| Risk | Likelihood | Impact | Mitigation |
|------|------------|--------|-----------|
| Breaking embedded platforms | Medium | High | Test AVR/STM32, monitor code size |
| Unintended behavior changes | Medium | Critical | Golden vector testing, byte-for-byte comparison |
| Performance regression | Low-Medium | Medium | Benchmark hot paths, profile if needed |
| Code size increase | Medium | Medium | Measure before/after, use -Os optimization |
| Breaking public API | Low | Critical | Never modify public headers |
| Introducing new bugs | Low-Medium | High | Code review, full test suite, Valgrind |
| Maintainer rejection | Medium | High | Get buy-in early, show value, iterate on feedback |

---

## 7. Style Guide Proposal

### Naming Conventions
- **Functions:** `pb_<module>_<action>` (public), `<action>_<noun>` (internal)
- **Variables:** Descriptive names, avoid Hungarian notation
- **Constants:** `UPPER_SNAKE_CASE`
- **Types:** Suffix `_t`

### Code Patterns
- **Error handling:** Early returns, consistent `PB_RETURN_ERROR`
- **Null checks:** Always check before dereferencing
- **Comments:** Explain why, not what
- **Macros vs inline:** Prefer inline for type safety

---

## 8. Next Steps

1. ✅ Get maintainer feedback on this roadmap
2. Set up verification infrastructure (golden vectors)
3. Start Phase 1, Commit 1.1 (documentation)
4. Iterate based on review feedback
5. Track progress using this roadmap

---

**Status:** Ready for implementation  
**Next Action:** Begin Phase 1, Commit 1.1
