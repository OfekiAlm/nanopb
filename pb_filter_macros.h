/* pb_filter_macros.h: Filter integration macros for nanopb validation
 *
 * This header provides optional integration with an external filter module.
 * When a filter.h is detected (or PB_FORCE_FILTER is defined), these macros
 * will call filter logging/error functions. Otherwise, they expand to
 * minimal or no-op implementations.
 *
 * Configuration:
 *   PB_FORCE_FILTER    - Force filter integration (define PB_HAS_FILTER)
 *   PB_FORCE_NO_FILTER - Disable filter integration
 *   PB_HAS_FILTER      - Manual control of filter availability
 *
 * Operating Modes:
 *   Mode A (PB_HAS_FILTER): Error macros call filter_error_* functions,
 *                          check macros log via filter_check_*, and
 *                          PB_LOG_* emits filter_error.
 *   Mode B (no filter):    Error macros return false, check macros are
 *                          pure comparisons, PB_LOG_* are no-ops.
 *
 * Available Macros:
 *   PB_ERROR_BUFFER_TOO_SMALL(ctx, struct_size, packet_size)
 *   PB_ERROR_BUFFER_TOO_BIG(ctx, struct_size, packet_size)
 *   PB_CHECK_EQ(ctx, var, exact)
 *   PB_CHECK_RANGE(ctx, var, minv, maxv)
 *   PB_CHECK_MIN(ctx, var, minv)
 *   PB_CHECK_MAX(ctx, var, maxv)
 *   PB_VALIDATE_PACKET_SIZE(ctx, struct_size, packet_size)
 *   PB_LOG_INVALID_OPCODE(ctx)
 *   PB_LOG_INVALID_HEADER(ctx)
 */

#ifndef PB_FILTER_MACROS_H_INCLUDED
#define PB_FILTER_MACROS_H_INCLUDED

/* Configuration priority: PB_FORCE_NO_FILTER > PB_FORCE_FILTER > auto-detect */
#ifdef PB_FORCE_NO_FILTER
#  undef PB_HAS_FILTER
#endif

#ifdef PB_FORCE_FILTER
#  ifndef PB_HAS_FILTER
#    define PB_HAS_FILTER 1
#  endif
#endif

/* Auto-detect filter.h availability using __has_include if available */
#if !defined(PB_HAS_FILTER) && !defined(PB_FORCE_NO_FILTER)
#  if defined(__has_include)
#    if __has_include("filter.h")
#      define PB_HAS_FILTER 1
#    endif
#  endif
#endif

/* Include filter header if available */
#ifdef PB_HAS_FILTER
#  include "filter.h"
#endif

/*
 * Internal filter call wrappers
 * These are used by the public macros below
 */
#ifdef PB_HAS_FILTER
#  define PB_FILTER_CALL_ERROR_PACKET_SIZE(ctx, errstr, packet_size, allowed_size) \
       (void)filter_error_packet_size((ctx), (errstr), (packet_size), (allowed_size))
#  define PB_FILTER_CALL_VALUE(ctx, var, exact) \
       (void)filter_check_value_generic((ctx), (var), (exact))
#  define PB_FILTER_CALL_BOUNDARY(ctx, var, minv, maxv) \
       (void)filter_check_boundary_generic((ctx), (var), (minv), (maxv))
#  define PB_FILTER_CALL_MIN(ctx, var, minv) \
       (void)filter_check_min_generic((ctx), (var), (minv))
#  define PB_FILTER_CALL_MAX(ctx, var, maxv) \
       (void)filter_check_max_generic((ctx), (var), (maxv))
#  define PB_FILTER_CALL_ERROR(ctx, errstr) \
       (void)filter_error((ctx), (errstr))
#else
#  define PB_FILTER_CALL_ERROR_PACKET_SIZE(ctx, errstr, packet_size, allowed_size) ((void)0)
#  define PB_FILTER_CALL_VALUE(ctx, var, exact)                                     ((void)0)
#  define PB_FILTER_CALL_BOUNDARY(ctx, var, minv, maxv)                             ((void)0)
#  define PB_FILTER_CALL_MIN(ctx, var, minv)                                        ((void)0)
#  define PB_FILTER_CALL_MAX(ctx, var, maxv)                                        ((void)0)
#  define PB_FILTER_CALL_ERROR(ctx, errstr)                                         ((void)0)
#endif

/*
 * Buffer size error macros
 * These report an error and return false from the calling function
 */
#ifdef PB_HAS_FILTER
#  define PB_ERROR_BUFFER_TOO_SMALL(ctx, struct_size, packet_size) \
       do { \
           PB_FILTER_CALL_ERROR_PACKET_SIZE((ctx), "buffer_too_small", (packet_size), (struct_size)); \
           return false; \
       } while (0)
#  define PB_ERROR_BUFFER_TOO_BIG(ctx, struct_size, packet_size) \
       do { \
           PB_FILTER_CALL_ERROR_PACKET_SIZE((ctx), "buffer_too_big", (packet_size), (struct_size)); \
           return false; \
       } while (0)
#else
#  define PB_ERROR_BUFFER_TOO_SMALL(ctx, struct_size, packet_size) \
       do { (void)(ctx); (void)(struct_size); (void)(packet_size); return false; } while (0)
#  define PB_ERROR_BUFFER_TOO_BIG(ctx, struct_size, packet_size) \
       do { (void)(ctx); (void)(struct_size); (void)(packet_size); return false; } while (0)
#endif

/*
 * Check macros - return boolean result, optionally log to filter
 */
#ifdef PB_HAS_FILTER
#  define PB_CHECK_EQ(ctx, var, exact) \
       (PB_FILTER_CALL_VALUE((ctx), (var), (exact)), ((var) == (exact)))
#  define PB_CHECK_RANGE(ctx, var, minv, maxv) \
       (PB_FILTER_CALL_BOUNDARY((ctx), (var), (minv), (maxv)), ((var) >= (minv) && (var) <= (maxv)))
#  define PB_CHECK_MIN(ctx, var, minv) \
       (PB_FILTER_CALL_MIN((ctx), (var), (minv)), ((var) >= (minv)))
#  define PB_CHECK_MAX(ctx, var, maxv) \
       (PB_FILTER_CALL_MAX((ctx), (var), (maxv)), ((var) <= (maxv)))
#else
#  define PB_CHECK_EQ(ctx, var, exact)         ((void)(ctx), ((var) == (exact)))
#  define PB_CHECK_RANGE(ctx, var, minv, maxv) ((void)(ctx), ((var) >= (minv) && (var) <= (maxv)))
#  define PB_CHECK_MIN(ctx, var, minv)         ((void)(ctx), ((var) >= (minv)))
#  define PB_CHECK_MAX(ctx, var, maxv)         ((void)(ctx), ((var) <= (maxv)))
#endif

/*
 * Packet size validation macro
 * Checks if packet_size matches struct_size, reports error if not
 */
#define PB_VALIDATE_PACKET_SIZE(ctx, struct_size, packet_size) \
    do { \
        if ((packet_size) < (struct_size)) { \
            PB_ERROR_BUFFER_TOO_SMALL((ctx), (struct_size), (packet_size)); \
        } else if ((packet_size) > (struct_size)) { \
            PB_ERROR_BUFFER_TOO_BIG((ctx), (struct_size), (packet_size)); \
        } \
    } while (0)

/*
 * Logging macros for protocol errors
 */
#ifdef PB_HAS_FILTER
#  define PB_LOG_INVALID_OPCODE(ctx) do { PB_FILTER_CALL_ERROR((ctx), "invalid_opcode"); } while (0)
#  define PB_LOG_INVALID_HEADER(ctx) do { PB_FILTER_CALL_ERROR((ctx), "invalid_header"); } while (0)
#else
#  define PB_LOG_INVALID_OPCODE(ctx) do { (void)(ctx); } while (0)
#  define PB_LOG_INVALID_HEADER(ctx) do { (void)(ctx); } while (0)
#endif

#endif /* PB_FILTER_MACROS_H_INCLUDED */
