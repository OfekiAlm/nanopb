/* Validation implementation for proto_examples/simple.proto */
#include "proto_examples/simple_validate.h"
#include <string.h>

bool pb_validate_test_SimpleMessage(const test_SimpleMessage *msg, pb_violations_t *violations)
{
    PB_VALIDATE_BEGIN(ctx, test_SimpleMessage, msg, violations);

    /* Validate field: bounded_float */
    PB_VALIDATE_FIELD_BEGIN(ctx, "bounded_float");
    {
        /* Rule: float.lte */
        PB_VALIDATE_NUMERIC_GENERIC(ctx, msg, bounded_float, float, pb_validate_float, PB_VALIDATE_RULE_LTE, 100.0, "float.lte");
    }
    {
        /* Rule: float.gte */
        PB_VALIDATE_NUMERIC_GENERIC(ctx, msg, bounded_float, float, pb_validate_float, PB_VALIDATE_RULE_GTE, 0.0, "float.gte");
    }
    PB_VALIDATE_FIELD_END(ctx);
    
    PB_VALIDATE_END(ctx, violations);
}
