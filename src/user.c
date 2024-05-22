#include <stdlib.h>
#include <string.h>
#include "user.h"
#include <debug.h>

struct user {
    char *handle;  // The handle of the user
    int ref_count; // Reference count for the user object
};

USER *user_create(char *handle) {
    if (handle == NULL) {
        return NULL;
    }
    // Allocate memory for the user object
    USER *user = (USER *)malloc(sizeof(USER));
    if (user == NULL) {
        return NULL;
    }
    // Allocate memory for the handle and copy it
    user->handle = strdup(handle);
    if (user->handle == NULL) {
        free(user->handle);
        free(user);
        return NULL;
    }

    user->ref_count = 1;
    return user;
}

USER *user_ref(USER *user, char *why) {
    if (user != NULL) {
        user->ref_count++;
        debug("User ref count: (%d -> %d)", user -> ref_count - 1, user -> ref_count);
    }
    return user;
}

void user_unref(USER *user, char *why) {
    if (user != NULL) {
        user->ref_count--;
        debug("User ref count: (%d -> %d)", user -> ref_count + 1, user -> ref_count);
        if (user->ref_count == 0) {
            // Free the handle and user object
            debug("Free User");
            free(user->handle);
            free(user);
        }
    }
}

char *user_get_handle(USER *user) {
    if (user != NULL) {
        return user->handle;
    }
    return NULL;
}
