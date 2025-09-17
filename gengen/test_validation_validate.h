/* Validation header for test_validation.proto */
#ifndef PB_VALIDATE_TEST_VALIDATION_INCLUDED
#define PB_VALIDATE_TEST_VALIDATION_INCLUDED

#include <pb_validate.h>
#include "test_validation.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Validate test_NumericConstraints message */
bool pb_validate_test_NumericConstraints(const test_NumericConstraints *msg, pb_violations_t *violations);

/* Validate test_StringConstraints message */
bool pb_validate_test_StringConstraints(const test_StringConstraints *msg, pb_violations_t *violations);

/* Validate test_BytesConstraints message */
bool pb_validate_test_BytesConstraints(const test_BytesConstraints *msg, pb_violations_t *violations);

/* Validate test_BoolConstraints message */
bool pb_validate_test_BoolConstraints(const test_BoolConstraints *msg, pb_violations_t *violations);

/* Validate test_EnumConstraints message */
bool pb_validate_test_EnumConstraints(const test_EnumConstraints *msg, pb_violations_t *violations);

/* Validate test_RepeatedConstraints message */
bool pb_validate_test_RepeatedConstraints(const test_RepeatedConstraints *msg, pb_violations_t *violations);

/* Validate test_MapConstraints message */
bool pb_validate_test_MapConstraints(const test_MapConstraints *msg, pb_violations_t *violations);

/* Validate test_MapConstraints_MinPairsMapEntry message */
bool pb_validate_test_MapConstraints_MinPairsMapEntry(const test_MapConstraints_MinPairsMapEntry *msg, pb_violations_t *violations);

/* Validate test_MapConstraints_MaxPairsMapEntry message */
bool pb_validate_test_MapConstraints_MaxPairsMapEntry(const test_MapConstraints_MaxPairsMapEntry *msg, pb_violations_t *violations);

/* Validate test_MapConstraints_NoSparseMapEntry message */
bool pb_validate_test_MapConstraints_NoSparseMapEntry(const test_MapConstraints_NoSparseMapEntry *msg, pb_violations_t *violations);

/* Validate test_OneofConstraints message */
bool pb_validate_test_OneofConstraints(const test_OneofConstraints *msg, pb_violations_t *violations);

/* Validate test_MessageConstraints message */
bool pb_validate_test_MessageConstraints(const test_MessageConstraints *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_TEST_VALIDATION_INCLUDED */
