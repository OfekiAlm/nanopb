/* Validation implementation for test_validation.proto */
#include "test_validation_validate.h"
#include <string.h>

bool pb_validate_test_NumericConstraints(const test_NumericConstraints *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_StringConstraints(const test_StringConstraints *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_BytesConstraints(const test_BytesConstraints *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_BoolConstraints(const test_BoolConstraints *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_EnumConstraints(const test_EnumConstraints *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_RepeatedConstraints(const test_RepeatedConstraints *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_MapConstraints(const test_MapConstraints *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_MapConstraints_MinPairsMapEntry(const test_MapConstraints_MinPairsMapEntry *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_MapConstraints_MaxPairsMapEntry(const test_MapConstraints_MaxPairsMapEntry *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_MapConstraints_NoSparseMapEntry(const test_MapConstraints_NoSparseMapEntry *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_OneofConstraints(const test_OneofConstraints *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_MessageConstraints(const test_MessageConstraints *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

