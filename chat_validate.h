/* Validation header for chat.proto */
#ifndef PB_VALIDATE_CHAT_INCLUDED
#define PB_VALIDATE_CHAT_INCLUDED

#include <pb_validate.h>
#include "chat.pb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Validate chat_ClientMessage message */
bool pb_validate_chat_ClientMessage(const chat_ClientMessage *msg, pb_violations_t *violations);

/* Validate chat_ServerMessage message */
bool pb_validate_chat_ServerMessage(const chat_ServerMessage *msg, pb_violations_t *violations);

/* Validate chat_Notification message */
bool pb_validate_chat_Notification(const chat_Notification *msg, pb_violations_t *violations);

/* Validate chat_StatusResponse message */
bool pb_validate_chat_StatusResponse(const chat_StatusResponse *msg, pb_violations_t *violations);

/* Validate chat_HistoryRequest message */
bool pb_validate_chat_HistoryRequest(const chat_HistoryRequest *msg, pb_violations_t *violations);

/* Validate chat_HistoryResponse message */
bool pb_validate_chat_HistoryResponse(const chat_HistoryResponse *msg, pb_violations_t *violations);

/* Validate chat_NotificationResponse message */
bool pb_validate_chat_NotificationResponse(const chat_NotificationResponse *msg, pb_violations_t *violations);

/* Validate chat_LoginRequest message */
bool pb_validate_chat_LoginRequest(const chat_LoginRequest *msg, pb_violations_t *violations);

/* Validate chat_LoginResponse message */
bool pb_validate_chat_LoginResponse(const chat_LoginResponse *msg, pb_violations_t *violations);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PB_VALIDATE_CHAT_INCLUDED */
