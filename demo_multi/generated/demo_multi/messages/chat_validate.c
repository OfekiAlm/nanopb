/* Validation implementation for demo_multi/messages/chat.proto */
#include "demo_multi/messages/chat_validate.h"
#include <string.h>

bool pb_validate_demo_messages_User(const demo_messages_User *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: id */
    if (!pb_validate_context_push_field(&ctx, "id")) return false;
    {
        /* Rule: string.min_len */
        {
            uint32_t min_len_v = 1;
            if (!pb_validate_string(msg->id, (pb_size_t)strlen(msg->id), &min_len_v, PB_VALIDATE_RULE_MIN_LEN)) {
                pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            uint32_t max_len_v = 36;
            if (!pb_validate_string(msg->id, (pb_size_t)strlen(msg->id), &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: name */
    if (!pb_validate_context_push_field(&ctx, "name")) return false;
    {
        /* Rule: string.min_len */
        {
            uint32_t min_len_v = 1;
            if (!pb_validate_string(msg->name, (pb_size_t)strlen(msg->name), &min_len_v, PB_VALIDATE_RULE_MIN_LEN)) {
                pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            uint32_t max_len_v = 50;
            if (!pb_validate_string(msg->name, (pb_size_t)strlen(msg->name), &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

bool pb_validate_demo_messages_ChatMessage(const demo_messages_ChatMessage *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: text */
    if (!pb_validate_context_push_field(&ctx, "text")) return false;
    {
        /* Rule: string.min_len */
        {
            uint32_t min_len_v = 1;
            if (!pb_validate_string(msg->text, (pb_size_t)strlen(msg->text), &min_len_v, PB_VALIDATE_RULE_MIN_LEN)) {
                pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: string.max_len */
        {
            uint32_t max_len_v = 256;
            if (!pb_validate_string(msg->text, (pb_size_t)strlen(msg->text), &max_len_v, PB_VALIDATE_RULE_MAX_LEN)) {
                pb_violations_add(violations, ctx.path_buffer, "string.max_len", "String too long");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: theme */
    if (!pb_validate_context_push_field(&ctx, "theme")) return false;
    {
        /* Rule: enum.defined_only */
        /* TODO: Implement enum defined_only validation */
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: from */
    if (!pb_validate_context_push_field(&ctx, "from")) return false;
    {
        /* Recurse into submessage (optional) */
        if (msg->has_from) {
            bool ok_nested = pb_validate_demo_messages_User(&msg->from, violations);
            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: to */
    if (!pb_validate_context_push_field(&ctx, "to")) return false;
    {
        /* Recurse into submessage (optional) */
        if (msg->has_to) {
            bool ok_nested = pb_validate_demo_messages_User(&msg->to, violations);
            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: location */
    if (!pb_validate_context_push_field(&ctx, "location")) return false;
    {
        /* Recurse into submessage (optional) */
        if (msg->has_location) {
            bool ok_nested = pb_validate_demo_structures_ColoredPoint(&msg->location, violations);
            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: sent_at */
    if (!pb_validate_context_push_field(&ctx, "sent_at")) return false;
    {
        /* Recurse into submessage (optional) */
        if (msg->has_sent_at) {
            bool ok_nested = pb_validate_google_protobuf_Timestamp(&msg->sent_at, violations);
            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    return !pb_violations_has_any(violations);
}

