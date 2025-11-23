/* Validation implementation for test_oneof_envelope.proto */
#include "test_oneof_envelope_validate.h"
#include <string.h>

bool pb_validate_test_LoginRequest(const test_LoginRequest *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - password
       - username
    */

    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_LogoutRequest(const test_LogoutRequest *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - session_id
    */

    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_ChatMessage(const test_ChatMessage *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - content
       - timestamp
    */

    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Envelope(const test_Envelope *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - payload
       - timestamp
       - type
    */

    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    return !pb_violations_has_any(violations);
}

