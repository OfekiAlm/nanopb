#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "pb_validate.h"

#include "gengen/test_basic_validation.pb.h"
#include "gengen/test_basic_validation_validate.h"

static void print_violations(const pb_violations_t *viol)
{
    printf("violations: %u (truncated=%s)\n", (unsigned)pb_violations_count(viol), viol->truncated ? "true" : "false");
    for (pb_size_t i = 0; i < pb_violations_count(viol); ++i)
    {
        const pb_violation_t *v = &viol->violations[i];
        printf("- %s: %s (%s)\n", v->field_path ? v->field_path : "<path>", v->message ? v->message : "<msg>", v->constraint_id ? v->constraint_id : "<rule>");
    }
}

static void print_check(const char *what)
{
    printf("\nTest: %s\n", what);
}

int main(void)
{
    printf("== Running validation tests ==\n");
    /* Happy path: valid message */
    test_BasicValidation msg = test_BasicValidation_init_zero;
    msg.age = 42;                /* 0 <= age <= 150 */
    msg.score = 1;               /* > 0 */
    msg.user_id = 123;           /* > 0 */
    msg.timestamp = 0;           /* >= 0 */
    msg.count = 7;               /* <= 1000 */
    msg.size = 50;               /* 10 <= size <= 100 */
    msg.has_total_bytes = false; /* optional, unset => skip */
    msg.sequence_num = 1;        /* >= 1 */
    msg.temperature = 25.5f;     /* -50 <= temperature <= 150 */
    msg.ratio = 0.5;             /* 0.0 < ratio < 1.0 */
    /* numbers repeated: 1..5 items */
    msg.numbers_count = 3;
    msg.numbers[0] = 1;
    msg.numbers[1] = 2;
    msg.numbers[2] = 3;

    /* Provide callback string values by setting arg pointer.
        We rely on validation helper that reads arg as const char*.
        (In a real decode scenario, user decode callback would populate arg.) */
    const char username_str[] = "user_ok";       /* 3..20 chars */
    const char email_str[] = "user@example.com"; /* contains '@' and len >=5 */
    const char password_str[] = "supersecret";   /* 8..100 */
    const char prefix_str[] = "PREFIX_";         /* exact boundary: just the prefix */
    const char suffix_str[] = "_SUFFIX";         /* exact boundary: just the suffix */
    const char ascii_ok[] = "ASCII123";          /* all ascii */
    const char color_ok[] = "red";               /* in set */
    const char forbidden_ok[] = "SAFE";          /* not in forbidden set */
    msg.username.arg = (void *)username_str;
    msg.email.arg = (void *)email_str;
    msg.password.arg = (void *)password_str;
    msg.prefix_field.arg = (void *)prefix_str;
    msg.suffix_field.arg = (void *)suffix_str;
    msg.ascii_field.arg = (void *)ascii_ok;
    msg.color_field.arg = (void *)color_ok;
    msg.forbidden_field.arg = (void *)forbidden_ok;
    /* New fields for semantic validation */
    const char email_fmt_ok[] = "alice@example.com";
    const char host_ok[] = "sub.example.org";
    const char ip_any_ok[] = "2001:db8::1";
    const char ipv4_ok[] = "192.168.1.10";
    const char ipv6_ok[] = "::1";
    msg.email_fmt.arg = (void *)email_fmt_ok;
    msg.hostname_fmt.arg = (void *)host_ok;
    msg.ip_any.arg = (void *)ip_any_ok;
    msg.ip_v4.arg = (void *)ipv4_ok;
    msg.ip_v6.arg = (void *)ipv6_ok;

    pb_violations_t viol;
    pb_violations_init(&viol);
    print_check("Happy path: all fields valid (expect no violations)");
    bool ok = pb_validate_test_BasicValidation(&msg, &viol);
    if (!ok)
    {
        printf("FAIL: expected valid message\n");
        print_violations(&viol);
        return 1;
    }
    if (pb_violations_has_any(&viol))
    {
        printf("FAIL: expected no violations (got %u)\n", (unsigned)pb_violations_count(&viol));
        print_violations(&viol);
        return 1;
    }
    printf("  -> PASS\n");

    /* Define helper to init a valid baseline message for negative tests */
#define INIT_BASELINE(varname)                                     \
    test_BasicValidation varname = test_BasicValidation_init_zero; \
    varname.age = 42;                                              \
    varname.score = 1;                                             \
    varname.user_id = 1;                                           \
    varname.timestamp = 0;                                         \
    varname.count = 7;                                             \
    varname.size = 50;                                             \
    varname.sequence_num = 1;                                      \
    varname.temperature = 25.0f;                                   \
    varname.ratio = 0.5;                                           \
    varname.numbers_count = 3;                                     \
    varname.numbers[0] = 1;                                        \
    varname.numbers[1] = 2;                                        \
    varname.numbers[2] = 3;                                        \
    varname.username.arg = (void *)username_str;                   \
    varname.email.arg = (void *)email_str;                         \
    varname.password.arg = (void *)password_str;                   \
    varname.prefix_field.arg = (void *)prefix_str;                 \
    varname.suffix_field.arg = (void *)suffix_str;                 \
    varname.ascii_field.arg = (void *)ascii_ok;                    \
    varname.color_field.arg = (void *)color_ok;                    \
    varname.forbidden_field.arg = (void *)forbidden_ok;            \
    varname.email_fmt.arg = (void *)email_fmt_ok;                  \
    varname.hostname_fmt.arg = (void *)host_ok;                    \
    varname.ip_any.arg = (void *)ip_any_ok;                        \
    varname.ip_v4.arg = (void *)ipv4_ok;                           \
    varname.ip_v6.arg = (void *)ipv6_ok;

    /* Negative test 1: numeric (float/double) violations (demonstrates early-exit or multi) */
    {
        test_BasicValidation bad = test_BasicValidation_init_zero;
        bad.age = 42;
        bad.score = 1;
        bad.user_id = 1;
        bad.timestamp = 0;
        bad.count = 7;
        bad.size = 50;
        bad.sequence_num = 1;
        bad.temperature = -100.0f;
        bad.ratio = 1.5;
        pb_violations_init(&viol);
        print_check("Numeric: temperature=-100.0 (>= -50), ratio=1.5 (< 1.0) — expect violations");
        ok = pb_validate_test_BasicValidation(&bad, &viol);
        if (ok || !pb_violations_has_any(&viol))
        {
            printf("FAIL: expected at least one violation\n");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* Repeated too few items (min_items=1) */
    {
        INIT_BASELINE(b_rep1);
        b_rep1.numbers_count = 0; /* too few */
        pb_violations_init(&viol);
        print_check("repeated numbers: too few items -> expect repeated.min_items");
        ok = pb_validate_test_BasicValidation(&b_rep1, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "repeated.min_items") != 0)
        {
            printf("FAIL: expected repeated.min_items (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* Repeated too many items (max_items=5) */
    {
        INIT_BASELINE(b_rep2);
        b_rep2.numbers_count = 6; /* exceeds max_count and max_items; only max_items is validated here */
        for (pb_size_t i = 0; i < 6 && i < (pb_size_t)(sizeof(b_rep2.numbers) / sizeof(b_rep2.numbers[0])); ++i)
            b_rep2.numbers[i] = (int32_t)i;
        pb_violations_init(&viol);
        print_check("repeated numbers: too many items -> expect repeated.max_items");
        ok = pb_validate_test_BasicValidation(&b_rep2, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "repeated.max_items") != 0)
        {
            printf("FAIL: expected repeated.max_items (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* Test each string rule individually so early-exit won't block later checks */

    /* Username min_len violation */
    {
        INIT_BASELINE(b1);
        static const char too_short[] = "ab"; /* <3 */
        b1.username.arg = (void *)too_short;
        pb_violations_init(&viol);
        print_check("username too short -> expect string.min_len");
        ok = pb_validate_test_BasicValidation(&b1, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.min_len") != 0)
        {
            printf("FAIL: expected string.min_len (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* Username max_len violation */
    {
        INIT_BASELINE(b2);
        static const char too_long[] = "this_username_is_way_too_long_for_validation"; /* >20 */
        b2.username.arg = (void *)too_long;
        pb_violations_init(&viol);
        print_check("username too long -> expect string.max_len");
        ok = pb_validate_test_BasicValidation(&b2, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.max_len") != 0)
        {
            printf("FAIL: expected string.max_len (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* Email contains '@' violation */
    {
        INIT_BASELINE(b3);
        static const char no_at[] = "userexample.com"; /* missing '@' */
        b3.email.arg = (void *)no_at;
        pb_violations_init(&viol);
        print_check("email missing '@' -> expect string.contains");
        ok = pb_validate_test_BasicValidation(&b3, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.contains") != 0)
        {
            printf("FAIL: expected string.contains (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* Email min_len violation */
    {
        INIT_BASELINE(b4);
        static const char too_short_email[] = "a@b"; /* <5 */
        b4.email.arg = (void *)too_short_email;
        pb_violations_init(&viol);
        print_check("email too short -> expect string.min_len");
        ok = pb_validate_test_BasicValidation(&b4, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.min_len") != 0)
        {
            printf("FAIL: expected string.min_len (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* Password min_len violation */
    {
        INIT_BASELINE(b5);
        static const char pw_short[] = "short"; /* <8 */
        b5.password.arg = (void *)pw_short;
        pb_violations_init(&viol);
        print_check("password too short -> expect string.min_len");
        ok = pb_validate_test_BasicValidation(&b5, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.min_len") != 0)
        {
            printf("FAIL: expected string.min_len (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* Prefix violation */
    {
        INIT_BASELINE(b6);
        static const char wrong_prefix[] = "PRE_value"; /* does not start with PREFIX_ */
        b6.prefix_field.arg = (void *)wrong_prefix;
        pb_violations_init(&viol);
        print_check("wrong prefix -> expect string.prefix");
        ok = pb_validate_test_BasicValidation(&b6, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.prefix") != 0)
        {
            printf("FAIL: expected string.prefix (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* Suffix violation */
    {
        INIT_BASELINE(b7);
        static const char wrong_suffix[] = "value_SUFF"; /* missing IX */
        b7.suffix_field.arg = (void *)wrong_suffix;
        pb_violations_init(&viol);
        print_check("wrong suffix -> expect string.suffix");
        ok = pb_validate_test_BasicValidation(&b7, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.suffix") != 0)
        {
            printf("FAIL: expected string.suffix (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* ASCII violation */
    {
        INIT_BASELINE(b8);
        static const char non_ascii[] = "caf\xC3\xA9"; /* contains UTF-8 bytes >127 */
        b8.ascii_field.arg = (void *)non_ascii;
        pb_violations_init(&viol);
        print_check("non-ASCII bytes -> expect string.ascii");
        ok = pb_validate_test_BasicValidation(&b8, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.ascii") != 0)
        {
            printf("FAIL: expected string.ascii (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* IN set violation */
    {
        INIT_BASELINE(b9);
        static const char wrong_color[] = "purple"; /* not in {red,green,blue} */
        b9.color_field.arg = (void *)wrong_color;
        pb_violations_init(&viol);
        print_check("color not in allowed set -> expect string.in");
        ok = pb_validate_test_BasicValidation(&b9, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.in") != 0)
        {
            printf("FAIL: expected string.in (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* NOT_IN violation */
    {
        INIT_BASELINE(b10);
        static const char blocked_word[] = "DELETE"; /* forbidden */
        b10.forbidden_field.arg = (void *)blocked_word;
        pb_violations_init(&viol);
        print_check("forbidden word -> expect string.not_in");
        ok = pb_validate_test_BasicValidation(&b10, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.not_in") != 0)
        {
            printf("FAIL: expected string.not_in (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* Email format violation */
    {
        INIT_BASELINE(b11);
        static const char bad_email[] = "invalid-at-domain";
        b11.email_fmt.arg = (void *)bad_email;
        pb_violations_init(&viol);
        print_check("email_fmt invalid -> expect string.email");
        ok = pb_validate_test_BasicValidation(&b11, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.email") != 0)
        {
            printf("FAIL: expected string.email (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* Hostname format violation */
    {
        INIT_BASELINE(b12);
        static const char bad_host[] = "-bad.example";
        b12.hostname_fmt.arg = (void *)bad_host;
        pb_violations_init(&viol);
        print_check("hostname_fmt invalid -> expect string.hostname");
        ok = pb_validate_test_BasicValidation(&b12, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.hostname") != 0)
        {
            printf("FAIL: expected string.hostname (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* IP (any) format violation */
    {
        INIT_BASELINE(b13);
        static const char bad_ip_any[] = "300.0.0.1";
        b13.ip_any.arg = (void *)bad_ip_any;
        pb_violations_init(&viol);
        print_check("ip_any invalid -> expect string.ip");
        ok = pb_validate_test_BasicValidation(&b13, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.ip") != 0)
        {
            printf("FAIL: expected string.ip (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* IPv4 format violation */
    {
        INIT_BASELINE(b14);
        static const char bad_v4[] = "1.2.3";
        b14.ip_v4.arg = (void *)bad_v4;
        pb_violations_init(&viol);
        print_check("ip_v4 invalid -> expect string.ipv4");
        ok = pb_validate_test_BasicValidation(&b14, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.ipv4") != 0)
        {
            printf("FAIL: expected string.ipv4 (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

    /* IPv6 format violation */
    {
        INIT_BASELINE(b15);
        static const char bad_v6[] = "2001:::1";
        b15.ip_v6.arg = (void *)bad_v6;
        pb_violations_init(&viol);
        print_check("ip_v6 invalid -> expect string.ipv6");
        ok = pb_validate_test_BasicValidation(&b15, &viol);
        if (ok || !pb_violations_has_any(&viol) || strcmp(viol.violations[0].constraint_id, "string.ipv6") != 0)
        {
            printf("FAIL: expected string.ipv6 (got %s)\n", pb_violations_has_any(&viol) ? viol.violations[0].constraint_id : "none");
            return 1;
        }
        printf("  -> PASS\n");
    }

#undef INIT_BASELINE

    /* Encode and decode the valid message to test pb functionality */
    uint8_t buffer[256];
    pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    print_check("Roundtrip encode/decode then re-validate (expect no violations)");
    if (!pb_encode(&ostream, &test_BasicValidation_msg, &msg))
    {
        printf("FAIL: encode: %s\n", PB_GET_ERROR(&ostream));
        return 1;
    }
    size_t encoded_size = ostream.bytes_written;
    printf("  -> PASS (encoded, %zu bytes)\n", encoded_size);

    test_BasicValidation round = test_BasicValidation_init_zero;
    pb_istream_t istream = pb_istream_from_buffer(buffer, encoded_size);
    if (!pb_decode(&istream, &test_BasicValidation_msg, &round))
    {
        printf("FAIL: decode: %s\n", PB_GET_ERROR(&istream));
        return 1;
    }
    printf("  -> PASS (decoded)\n");

    /* Re-validate decoded */
    pb_violations_init(&viol);
    ok = pb_validate_test_BasicValidation(&round, &viol);
    if (!ok || pb_violations_has_any(&viol))
    {
        printf("FAIL: re-validate after decode\n");
        print_violations(&viol);
        return 1;
    }
    printf("  -> PASS (re-validated)\n");

    printf("\n== All tests passed ==\n");
    return 0;
}
