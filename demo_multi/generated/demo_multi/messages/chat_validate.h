/* Validation header for demo_multi/messages/chat.proto */
#ifndef PB_VALIDATE_DEMO_MULTI_MESSAGES_CHAT_INCLUDED
#define PB_VALIDATE_DEMO_MULTI_MESSAGES_CHAT_INCLUDED

#include <pb_validate.h>
#include "demo_multi/messages/chat.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Validate demo_messages_User message */
bool pb_validate_demo_messages_User(const demo_messages_User *msg, pb_violations_t *violations);

/* Validate demo_messages_ChatMessage message */
bool pb_validate_demo_messages_ChatMessage(const demo_messages_ChatMessage *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_DEMO_MULTI_MESSAGES_CHAT_INCLUDED */
