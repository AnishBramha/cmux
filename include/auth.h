#pragma once

#include "common.h"

#define __USERS_MAX__        256

#define __USERNAME_ADMIN__   "admin"
#define __PASSWORD_ADMIN__   "12345"

#define __USERNAME_LEN_MAX__ 256
#define __PASSWORD_LEN_MAX__ 256
#define __FILES_OWNED_MAX__  256
#define __FILENAME_LEN_MAX__ 256


typedef enum {

    ADMIN,
    CLIENT,

} Role;


typedef struct {

    char username[__USERNAME_LEN_MAX__];
    char password[__PASSWORD_LEN_MAX__];
    Role role;
    char files[__FILES_OWNED_MAX__][__FILENAME_LEN_MAX__];

} Record;


typedef enum {

    PKT_LOGIN_REQ,
    PKT_LOGIN_RES,

} PacketType;


typedef struct {

    PacketType type;
    char username[__USERNAME_LEN_MAX__];
    char password[__PASSWORD_LEN_MAX__];

} LoginRequest;


typedef struct {

    PacketType type;
    bool success;
    Role role;
    char msg[__ERRMSG_LEN_MAX__];

} LoginResponse;






