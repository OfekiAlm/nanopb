/* Validation implementation for chat.proto */
#include "chat_validate.h"
#include <string.h>

bool pb_validate_chat_ClientMessage(const chat_ClientMessage *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: user */
    if (!pb_validate_context_push_field(&ctx, "user")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->user, &s, &l)) {
                uint32_t min_len = 3;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->user, &s, &l)) {
                uint32_t max_len_v = 20;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: text */
    if (!pb_validate_context_push_field(&ctx, "text")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->text, &s, &l)) {
                uint32_t min_len = 1;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->text, &s, &l)) {
                uint32_t max_len_v = 500;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* AUTO_RECURSE: begin */
    /* AUTO_RECURSE: end */
    return !pb_violations_has_any(violations);
}

bool pb_validate_chat_ServerMessage(const chat_ServerMessage *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: user */
    if (!pb_validate_context_push_field(&ctx, "user")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->user, &s, &l)) {
                uint32_t min_len = 3;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->user, &s, &l)) {
                uint32_t max_len_v = 20;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: text */
    if (!pb_validate_context_push_field(&ctx, "text")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->text, &s, &l)) {
                uint32_t min_len = 1;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->text, &s, &l)) {
                uint32_t max_len_v = 500;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: timestamp */
    if (!pb_validate_context_push_field(&ctx, "timestamp")) return false;
    {
        /* Rule: int64.gte */
        {
            int64_t expected = (int64_t)0;
            if (!pb_validate_int64(msg->timestamp, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "int64.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* AUTO_RECURSE: begin */
    /* AUTO_RECURSE: end */
    return !pb_violations_has_any(violations);
}

bool pb_validate_chat_Notification(const chat_Notification *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: title */
    if (!pb_validate_context_push_field(&ctx, "title")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->title, &s, &l)) {
                uint32_t min_len = 1;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->title, &s, &l)) {
                uint32_t max_len_v = 100;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: body */
    if (!pb_validate_context_push_field(&ctx, "body")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->body, &s, &l)) {
                uint32_t min_len = 1;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->body, &s, &l)) {
                uint32_t max_len_v = 1000;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* AUTO_RECURSE: begin */
    /* AUTO_RECURSE: end */
    return !pb_violations_has_any(violations);
}

bool pb_validate_chat_StatusResponse(const chat_StatusResponse *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: status */
    if (!pb_validate_context_push_field(&ctx, "status")) return false;
    {
        /* Rule: string.in */
        {
            const char *s = NULL; pb_size_t l = 0; (void)l;
            const char *allowed[] = { "OK", "ERROR", "WARN" };
            if (pb_read_callback_string(&msg->status, &s, &l)) {
                bool match = false;
                for (size_t i = 0; i < sizeof(allowed)/sizeof(allowed[0]); i++) {
                    if (strcmp(s, allowed[i]) == 0) { match = true; break; }
                }
                if (!match) {
                    pb_violations_add(violations, ctx.path_buffer, "string.in", "Value must be one of allowed set");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: message */
    if (!pb_validate_context_push_field(&ctx, "message")) return false;
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->message, &s, &l)) {
                uint32_t max_len_v = 200;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* AUTO_RECURSE: begin */
    /* AUTO_RECURSE: end */
    return !pb_violations_has_any(violations);
}

bool pb_validate_chat_HistoryRequest(const chat_HistoryRequest *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: user */
    if (!pb_validate_context_push_field(&ctx, "user")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->user, &s, &l)) {
                uint32_t min_len = 3;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->user, &s, &l)) {
                uint32_t max_len_v = 20;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: since_timestamp */
    if (!pb_validate_context_push_field(&ctx, "since_timestamp")) return false;
    {
        /* Rule: int64.gte */
        {
            int64_t expected = (int64_t)0;
            if (!pb_validate_int64(msg->since_timestamp, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "int64.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* AUTO_RECURSE: begin */
    /* AUTO_RECURSE: end */
    return !pb_violations_has_any(violations);
}

bool pb_validate_chat_HistoryResponse(const chat_HistoryResponse *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: messages */
    if (!pb_validate_context_push_field(&ctx, "messages")) return false;
    {
        /* Rule: repeated.max_items */
        if (!pb_validate_max_items(msg->messages_count, 1000)) {
            pb_violations_add(violations, ctx.path_buffer, "repeated.max_items", "Too many items");
            if (ctx.early_exit) return false;
        }
    }
    {
        /* Recurse into submessage (singular) */
        bool ok_nested = pb_validate_chat_ServerMessage(&msg->messages, violations);
        if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* AUTO_RECURSE: begin */
    /* AUTO_RECURSE: end */
    return !pb_violations_has_any(violations);
}

bool pb_validate_chat_NotificationResponse(const chat_NotificationResponse *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: result */
    if (!pb_validate_context_push_field(&ctx, "result")) return false;
    {
        /* Rule: string.in */
        {
            const char *s = NULL; pb_size_t l = 0; (void)l;
            const char *allowed[] = { "SUCCESS", "FAILURE" };
            if (pb_read_callback_string(&msg->result, &s, &l)) {
                bool match = false;
                for (size_t i = 0; i < sizeof(allowed)/sizeof(allowed[0]); i++) {
                    if (strcmp(s, allowed[i]) == 0) { match = true; break; }
                }
                if (!match) {
                    pb_violations_add(violations, ctx.path_buffer, "string.in", "Value must be one of allowed set");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: message */
    if (!pb_validate_context_push_field(&ctx, "message")) return false;
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->message, &s, &l)) {
                uint32_t max_len_v = 200;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* AUTO_RECURSE: begin */
    /* AUTO_RECURSE: end */
    return !pb_violations_has_any(violations);
}

bool pb_validate_chat_LoginRequest(const chat_LoginRequest *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: username */
    if (!pb_validate_context_push_field(&ctx, "username")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->username, &s, &l)) {
                uint32_t min_len = 3;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->username, &s, &l)) {
                uint32_t max_len_v = 20;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: password */
    if (!pb_validate_context_push_field(&ctx, "password")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->password, &s, &l)) {
                uint32_t min_len = 6;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->password, &s, &l)) {
                uint32_t max_len_v = 100;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* AUTO_RECURSE: begin */
    /* AUTO_RECURSE: end */
    return !pb_violations_has_any(violations);
}

bool pb_validate_chat_LoginResponse(const chat_LoginResponse *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: message */
    if (!pb_validate_context_push_field(&ctx, "message")) return false;
    {
        /* Rule: string.max_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->message, &s, &l)) {
                uint32_t max_len_v = 200;
                if (!pb_validate_string(s, l, &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* AUTO_RECURSE: begin */
    /* AUTO_RECURSE: end */
    return !pb_violations_has_any(violations);
}

