/* Validation implementation for test_any_validation.proto */
#include "test_any_validation_validate.h"
#include <string.h>

bool pb_validate_test_BaseMessage(const test_BaseMessage *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - message_id
       - timestamp
    */

    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: payload */
    if (!pb_validate_context_push_field(&ctx, "payload")) return false;
    {
        /* Rule: any.in */
        /* Validate Any type_url is in allowed list */
        if (msg->has_payload) {
            const char *type_url = (const char *)msg->payload.type_url;
            bool type_url_valid = false;
            if (type_url && strcmp(type_url, "type.googleapis.com/test.LoginRequest") == 0) type_url_valid = true;
            if (type_url && strcmp(type_url, "type.googleapis.com/test.LogoutRequest") == 0) type_url_valid = true;
            if (type_url && strcmp(type_url, "type.googleapis.com/test.ChatMessage") == 0) type_url_valid = true;
            if (!type_url_valid) {
                pb_violations_add(violations, ctx.path_buffer, "any.in", "type_url not in allowed list");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_RestrictedMessage(const test_RestrictedMessage *msg, pb_violations_t *violations)
{
    /* Fields without constraints:
       - message_id
    */

    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: payload */
    if (!pb_validate_context_push_field(&ctx, "payload")) return false;
    {
        /* Rule: any.not_in */
        /* Validate Any type_url is not in disallowed list */
        if (msg->has_payload) {
            const char *type_url = (const char *)msg->payload.type_url;
            if (type_url && strcmp(type_url, "type.googleapis.com/test.SensitiveData") == 0) {
                pb_violations_add(violations, ctx.path_buffer, "any.not_in", "type_url in disallowed list");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

