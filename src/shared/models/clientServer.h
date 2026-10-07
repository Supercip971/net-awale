#pragma once
#include "shared/models/const.h"

typedef enum { MSG_CONNECT, MSG_PLAYERS, MSG_PLAY, MSG_DEFY, MSG_ACCEPT_DEFY, MSG_DECLINE_DEFY } MsgType;

typedef struct {
    MsgType kind;
    union {
        struct { char pseudo[MAX_USERNAME_LENGTH]; } connect;
        struct { } players;
        struct { int hole; } play;
        struct { char pseudo[MAX_USERNAME_LENGTH]; } defy;
        struct { char pseudo[MAX_USERNAME_LENGTH]; } acceptDefy;
        struct { char pseudo[MAX_USERNAME_LENGTH]; } declineDefy;
    };
} ClientServerMessage;