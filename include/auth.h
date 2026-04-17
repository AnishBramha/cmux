#pragma once

#include "common.h"


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




