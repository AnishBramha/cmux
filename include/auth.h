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
    PKT_DIR_REQ,
    PKT_DIR_RES,
    PKT_FOPEN_REQ,
    PKT_FOPEN_RES,
    PKT_EDIT_REQ,
    PKT_SYNC_REQ,
    PKT_SYNC_RES,
    PKT_ERR,

} PacketType;


typedef enum {

    NODE_FILE,
    NODE_DIR,

} NodeType;


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


typedef struct {

    char name[__FILENAME_LEN_MAX__];
    NodeType type;
    int depth;

} FileNode;


typedef struct {

    PacketType type;
    char username[__USERNAME_LEN_MAX__];

} DirRequest;


typedef struct {

    PacketType type;
    int nnodes;
    FileNode nodes[__FILES_OWNED_MAX__];

} DirResponse;


typedef struct {

    PacketType type;
    char path[__FILENAME_LEN_MAX__];

} FOpenRequest;


typedef struct {

    PacketType type;
    bool success;
    size_t nlines;
    char lines[__FILE_LINES_MAX__][__PACKET_LEN_MAX__];

} FOpenResponse;


typedef struct {

    PacketType type;
    char path[__FILENAME_LEN_MAX__];
    int line;
    int col;
    char c;
    bool del;

} EditRequest;


typedef struct {

    PacketType type;
    char path[__FILENAME_LEN_MAX__];

} SyncRequest;

typedef struct {
    
    PacketType type;
    bool success;
    size_t nlines;
    char lines[__FILE_LINES_MAX__][__PACKET_LEN_MAX__];

} SyncResponse;







