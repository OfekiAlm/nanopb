/* Validation implementation for test_simple.proto */
#include "test_simple_validate.h"
#include <string.h>

bool pb_validate_test_SimpleRequest(const test_SimpleRequest *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_SimpleResponse(const test_SimpleResponse *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

