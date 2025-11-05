/* Validation header for demo_multi/structures/geometry.proto */
#ifndef PB_VALIDATE_DEMO_MULTI_STRUCTURES_GEOMETRY_INCLUDED
#define PB_VALIDATE_DEMO_MULTI_STRUCTURES_GEOMETRY_INCLUDED

#include <pb_validate.h>
#include "demo_multi/structures/geometry.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Validate demo_structures_Point message */
bool pb_validate_demo_structures_Point(const demo_structures_Point *msg, pb_violations_t *violations);

/* Validate demo_structures_ColoredPoint message */
bool pb_validate_demo_structures_ColoredPoint(const demo_structures_ColoredPoint *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_DEMO_MULTI_STRUCTURES_GEOMETRY_INCLUDED */
