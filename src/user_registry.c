#include <stdlib.h>
#include <string.h>
#include "user_registry.h"
#include "csapp.h"
#include "globals.h"
#include "debug.h"
#include "user.h"

typedef struct user_registry_entry {
    char *handle;
    USER *user;
    struct user_registry_entry *next;
} USER_REGISTRY_ENTRY;

struct user_registry {
    USER_REGISTRY_ENTRY *head;
    pthread_mutex_t mutex;
};

USER_REGISTRY *ureg_init(void) {
	debug("Starting registry intitialization");
    // Allocate memory for the user registry
    user_registry = (USER_REGISTRY *)malloc(sizeof(USER_REGISTRY));
    if (user_registry == NULL) {
        return NULL; // Memory allocation failed
    }

    // Initialize the head of the registry to NULL
    user_registry->head = NULL;

    // Initialize the mutex for thread safety
    pthread_mutex_init(&user_registry->mutex, NULL);
    debug("Registry initialized");

    return user_registry;
}

void ureg_fini(USER_REGISTRY *ureg) {
    // Lock the mutex before accessing the registry
    pthread_mutex_lock(&ureg->mutex);

    // Iterate through the registry and free all entries
    USER_REGISTRY_ENTRY *current = ureg->head;
    while (current != NULL) {
        USER_REGISTRY_ENTRY *next = current->next;
        free(current->handle);
        free(user_get_handle(current->user));
        free(current->user);
        free(current);
        current = next;
    }

    // Unlock the mutex and destroy it
    pthread_mutex_unlock(&ureg->mutex);
    pthread_mutex_destroy(&ureg->mutex);

    // Free the user registry object
    free(ureg);
}

USER *ureg_register(USER_REGISTRY *ureg, char *handle) {
	debug("Starting user registration");
    if (ureg == NULL || handle == NULL) {
        return NULL; // Invalid arguments
    }

    // Lock the mutex before accessing the registry
    pthread_mutex_lock(&ureg->mutex);

    // Check if the handle is already registered
    USER_REGISTRY_ENTRY *current = ureg->head;
    while (current != NULL) {
        if (strcmp(current->handle, handle) == 0) {
            // Increment the reference count and return the existing user
            pthread_mutex_unlock(&ureg->mutex);
            return user_ref(current->user, "Register existing user");
        }
        current = current->next;
    }
    debug("No user with this handle exists");

    // Create a new user object
    USER *new_user = user_create(handle);
    if (new_user == NULL) {
        pthread_mutex_unlock(&ureg->mutex);
        return NULL; // Failed to create a new user
    }
    debug("User created");

    // Create a new entry for the registry
    USER_REGISTRY_ENTRY *new_entry = (USER_REGISTRY_ENTRY *)malloc(sizeof(USER_REGISTRY_ENTRY));
    if (new_entry == NULL) {
        pthread_mutex_unlock(&ureg->mutex);
        return NULL; // Memory allocation failed
    }
    new_entry->handle = strdup(handle);
    new_entry->user = new_user;

    debug("Entry made");

    // Add the new entry to the head of the registry
    new_entry->next = ureg->head;
    ureg->head = new_entry;
    debug("Links made");

    // Unlock the mutex and return the new user
    pthread_mutex_unlock(&ureg->mutex);
    debug("User registered");
    return user_ref(new_entry->user, "New user: Pointer that is returned");;
}

void ureg_unregister(USER_REGISTRY *ureg, char *handle) {
    if (ureg == NULL || handle == NULL) {
        return; // Invalid arguments
    }

    // Lock the mutex before accessing the registry
    pthread_mutex_lock(&ureg->mutex);

    // Search for the entry with the given handle
    USER_REGISTRY_ENTRY *prev = NULL;
    USER_REGISTRY_ENTRY *current = ureg->head;
    while (current != NULL) {
        if (strcmp(current->handle, handle) == 0) {
            // Found the entry, unlink it from the registry
            if (prev == NULL) {
                ureg->head = current->next;
            } else {
                prev->next = current->next;
            }

            // Free resources associated with the entry
            user_unref(current->user, "Unregister");
            free(current->handle);
            free(current);

            // Unlock the mutex and return
            pthread_mutex_unlock(&ureg->mutex);
            return;
        }
        prev = current;
        current = current->next;
    }

    // Unlock the mutex if the handle was not found
    pthread_mutex_unlock(&ureg->mutex);
}