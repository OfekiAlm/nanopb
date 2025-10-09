/* Validation implementation for test_basic_validation.proto */
#include "test_basic_validation_validate.h"
#include <string.h>

bool pb_validate_test_BasicValidation(const test_BasicValidation *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: age */
    if (!pb_validate_context_push_field(&ctx, "age")) return false;
    {
        /* Rule: int32.gte */
        {
            int32_t expected = (int32_t)0;
            if (!pb_validate_int32(msg->age, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "int32.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: int32.lte */
        {
            int32_t expected = (int32_t)150;
            if (!pb_validate_int32(msg->age, &expected, PB_VALIDATE_RULE_LTE)) {
                pb_violations_add(violations, ctx.path_buffer, "int32.lte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: score */
    if (!pb_validate_context_push_field(&ctx, "score")) return false;
    {
        /* Rule: int32.gt */
        {
            int32_t expected = (int32_t)0;
            if (!pb_validate_int32(msg->score, &expected, PB_VALIDATE_RULE_GT)) {
                pb_violations_add(violations, ctx.path_buffer, "int32.gt", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: user_id */
    if (!pb_validate_context_push_field(&ctx, "user_id")) return false;
    {
        /* Rule: int64.gt */
        {
            int64_t expected = (int64_t)0;
            if (!pb_validate_int64(msg->user_id, &expected, PB_VALIDATE_RULE_GT)) {
                pb_violations_add(violations, ctx.path_buffer, "int64.gt", "Value constraint failed");
                if (ctx.early_exit) return false;
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
    
    /* Validate field: count */
    if (!pb_validate_context_push_field(&ctx, "count")) return false;
    {
        /* Rule: uint32.lte */
        {
            uint32_t expected = (uint32_t)1000;
            if (!pb_validate_uint32(msg->count, &expected, PB_VALIDATE_RULE_LTE)) {
                pb_violations_add(violations, ctx.path_buffer, "uint32.lte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: size */
    if (!pb_validate_context_push_field(&ctx, "size")) return false;
    {
        /* Rule: uint32.gte */
        {
            uint32_t expected = (uint32_t)10;
            if (!pb_validate_uint32(msg->size, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "uint32.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: uint32.lte */
        {
            uint32_t expected = (uint32_t)100;
            if (!pb_validate_uint32(msg->size, &expected, PB_VALIDATE_RULE_LTE)) {
                pb_violations_add(violations, ctx.path_buffer, "uint32.lte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: total_bytes */
    if (!pb_validate_context_push_field(&ctx, "total_bytes")) return false;
    {
        /* Rule: uint64.lt */
        if (msg->has_total_bytes) {
            {
                uint64_t expected = (uint64_t)1000000000;
                if (!pb_validate_uint64(msg->total_bytes, &expected, PB_VALIDATE_RULE_LT)) {
                    pb_violations_add(violations, ctx.path_buffer, "uint64.lt", "Value constraint failed");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: sequence_num */
    if (!pb_validate_context_push_field(&ctx, "sequence_num")) return false;
    {
        /* Rule: uint64.gte */
        {
            uint64_t expected = (uint64_t)1;
            if (!pb_validate_uint64(msg->sequence_num, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "uint64.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
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
    
    /* Validate field: email */
    if (!pb_validate_context_push_field(&ctx, "email")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->email, &s, &l)) {
                uint32_t min_len = 5;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.contains */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->email, &s, &l)) {
                const char *needle = "@";
                if (!pb_validate_string(s, l, needle, PB_VALIDATE_RULE_CONTAINS)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.contains", "String must contain '@'");
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
                uint32_t min_len = 8;
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
    
    /* Validate field: prefix_field */
    if (!pb_validate_context_push_field(&ctx, "prefix_field")) return false;
    {
        /* Rule: string.prefix */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->prefix_field, &s, &l)) {
                const char *prefix = "PREFIX_";
                if (!pb_validate_string(s, l, prefix, PB_VALIDATE_RULE_PREFIX)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.prefix", "String must start with 'PREFIX_'");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: suffix_field */
    if (!pb_validate_context_push_field(&ctx, "suffix_field")) return false;
    {
        /* Rule: string.suffix */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->suffix_field, &s, &l)) {
                const char *suffix = "_SUFFIX";
                if (!pb_validate_string(s, l, suffix, PB_VALIDATE_RULE_SUFFIX)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.suffix", "String must end with '_SUFFIX'");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: temperature */
    if (!pb_validate_context_push_field(&ctx, "temperature")) return false;
    {
        /* Rule: float.gte */
        {
            float expected = (float)-50.0;
            if (!pb_validate_float(msg->temperature, &expected, PB_VALIDATE_RULE_GTE)) {
                pb_violations_add(violations, ctx.path_buffer, "float.gte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: float.lte */
        {
            float expected = (float)150.0;
            if (!pb_validate_float(msg->temperature, &expected, PB_VALIDATE_RULE_LTE)) {
                pb_violations_add(violations, ctx.path_buffer, "float.lte", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: ratio */
    if (!pb_validate_context_push_field(&ctx, "ratio")) return false;
    {
        /* Rule: double.gt */
        {
            double expected = (double)0.0;
            if (!pb_validate_double(msg->ratio, &expected, PB_VALIDATE_RULE_GT)) {
                pb_violations_add(violations, ctx.path_buffer, "double.gt", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    {
        /* Rule: double.lt */
        {
            double expected = (double)1.0;
            if (!pb_validate_double(msg->ratio, &expected, PB_VALIDATE_RULE_LT)) {
                pb_violations_add(violations, ctx.path_buffer, "double.lt", "Value constraint failed");
                if (ctx.early_exit) return false;
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: ascii_field */
    if (!pb_validate_context_push_field(&ctx, "ascii_field")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->ascii_field, &s, &l)) {
                uint32_t min_len = 1;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    {
        /* Rule: string.ascii */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->ascii_field, &s, &l)) {
                if (!pb_validate_string(s, l, NULL, PB_VALIDATE_RULE_ASCII)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.ascii", "String must contain only ASCII characters");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: color_field */
    if (!pb_validate_context_push_field(&ctx, "color_field")) return false;
    {
        /* Rule: string.in */
        {
            const char *s = NULL; pb_size_t l = 0; (void)l;
            const char *allowed[] = { "red", "green", "blue" };
            if (pb_read_callback_string(&msg->color_field, &s, &l)) {
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
    
    /* Validate field: forbidden_field */
    if (!pb_validate_context_push_field(&ctx, "forbidden_field")) return false;
    {
        /* Rule: string.not_in */
        {
            const char *s = NULL; pb_size_t l = 0; (void)l;
            const char *blocked[] = { "DROP", "DELETE", "TRUNCATE" };
            if (pb_read_callback_string(&msg->forbidden_field, &s, &l)) {
                bool forbidden = false;
                for (size_t i = 0; i < sizeof(blocked)/sizeof(blocked[0]); i++) {
                    if (strcmp(s, blocked[i]) == 0) { forbidden = true; break; }
                }
                if (forbidden) {
                    pb_violations_add(violations, ctx.path_buffer, "string.not_in", "Value is in forbidden set");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: numbers */
    if (!pb_validate_context_push_field(&ctx, "numbers")) return false;
    {
        /* Rule: repeated.min_items */
        if (!pb_validate_min_items(msg->numbers_count, 1)) {
            pb_violations_add(violations, ctx.path_buffer, "repeated.min_items", "Too few items");
            if (ctx.early_exit) return false;
        }
    }
    {
        /* Rule: repeated.max_items */
        if (!pb_validate_max_items(msg->numbers_count, 5)) {
            pb_violations_add(violations, ctx.path_buffer, "repeated.max_items", "Too many items");
            if (ctx.early_exit) return false;
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: email_fmt */
    if (!pb_validate_context_push_field(&ctx, "email_fmt")) return false;
    {
        /* Rule: string.email */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->email_fmt, &s, &l)) {
                if (!pb_validate_string(s, l, NULL, PB_VALIDATE_RULE_EMAIL)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.email", "String format validation failed");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: hostname_fmt */
    if (!pb_validate_context_push_field(&ctx, "hostname_fmt")) return false;
    {
        /* Rule: string.hostname */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->hostname_fmt, &s, &l)) {
                if (!pb_validate_string(s, l, NULL, PB_VALIDATE_RULE_HOSTNAME)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.hostname", "String format validation failed");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: ip_any */
    if (!pb_validate_context_push_field(&ctx, "ip_any")) return false;
    {
        /* Rule: string.ip */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->ip_any, &s, &l)) {
                if (!pb_validate_string(s, l, NULL, PB_VALIDATE_RULE_IP)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.ip", "String format validation failed");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: ip_v4 */
    if (!pb_validate_context_push_field(&ctx, "ip_v4")) return false;
    {
        /* Rule: string.ipv4 */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->ip_v4, &s, &l)) {
                if (!pb_validate_string(s, l, NULL, PB_VALIDATE_RULE_IPV4)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.ipv4", "String format validation failed");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: ip_v6 */
    if (!pb_validate_context_push_field(&ctx, "ip_v6")) return false;
    {
        /* Rule: string.ipv6 */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->ip_v6, &s, &l)) {
                if (!pb_validate_string(s, l, NULL, PB_VALIDATE_RULE_IPV6)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.ipv6", "String format validation failed");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* AUTO_RECURSE: begin */
    /* Validate field: address */
    if (!pb_validate_context_push_field(&ctx, "address")) return false;
    {
        /* Recurse into submessage (optional) */
        if (msg->has_address) {
            bool ok_nested = pb_validate_test_Address(&msg->address, violations);
            if (!ok_nested && ctx.early_exit) { pb_validate_context_pop_field(&ctx); return false; }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* AUTO_RECURSE: end */
    return !pb_violations_has_any(violations);
}

bool pb_validate_test_Address(const test_Address *msg, pb_violations_t *violations)
{
    if (!msg) return false;
    
    pb_validate_context_t ctx = {0};
    ctx.violations = violations;
    ctx.early_exit = PB_VALIDATE_EARLY_EXIT;
    
    /* Validate field: city */
    if (!pb_validate_context_push_field(&ctx, "city")) return false;
    {
        /* Rule: string.min_len */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->city, &s, &l)) {
                uint32_t min_len = 1;
                if (!pb_validate_string(s, l, &min_len, PB_VALIDATE_RULE_MIN_LEN)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.min_len", "String too short");
                    if (ctx.early_exit) return false;
                }
            }
        }
    }
    pb_validate_context_pop_field(&ctx);
    
    /* Validate field: ip */
    if (!pb_validate_context_push_field(&ctx, "ip")) return false;
    {
        /* Rule: string.ip */
        {
            const char *s = NULL; pb_size_t l = 0;
            if (pb_read_callback_string(&msg->ip, &s, &l)) {
                if (!pb_validate_string(s, l, NULL, PB_VALIDATE_RULE_IP)) {
                    pb_violations_add(violations, ctx.path_buffer, "string.ip", "String format validation failed");
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

