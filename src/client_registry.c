#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "client_registry.h"
#include "csapp.h"
#include "debug.h"

// Define the structure for the client registry
struct client_registry {
    pthread_mutex_t mutex; // Mutex for thread safety
    int client_count;
    CLIENT *clients[MAX_CLIENTS]; // Array to store pointers to registered clients
    sem_t semaphore; // Semaphore to block until all clients are unregistered
};

CLIENT_REGISTRY *creg_init() {
    // Allocate memory for the client registry
    CLIENT_REGISTRY *cr = malloc(sizeof(CLIENT_REGISTRY));
    if (cr == NULL) {
        perror("Error: Unable to allocate memory for client registry");
        return NULL;
    }

    // Initialize mutex and semaphore
    pthread_mutex_init(&cr->mutex, NULL);
    sem_init(&cr->semaphore, 0, 0);

    // Initialize client count to zero
    cr->client_count = 0;

    debug("Client registry initialized\n");

    return cr;
}

void creg_fini(CLIENT_REGISTRY *cr) {
    if (cr == NULL) return;

    // Finalize each registered client
    for (int i = 0; i < cr->client_count; i++) {
        client_unref(cr->clients[i], "Finalizing client registry");
    }

    // Destroy mutex and semaphore
    pthread_mutex_destroy(&cr->mutex);
    sem_destroy(&cr->semaphore);

    // Free the client registry itself
    free(cr);
    debug("Client registry finalized\n");
}

CLIENT *creg_register(CLIENT_REGISTRY *cr, int fd) {
    if (cr == NULL) return NULL;

    // Create a new client object
    CLIENT *client = client_create(cr, fd);
    if (client == NULL) return NULL;

    pthread_mutex_lock(&cr->mutex);

    // Add the client to the registry if there's space
    if (cr->client_count < MAX_CLIENTS) {
        cr->clients[cr->client_count++] = client;
        client_ref(client, "Registering client");
        debug("Client registered\n");
    } else {
        // If the registry is full, free the client and return NULL
        client_unref(client, "Max clients reached");
        client = NULL;
        debug("Client registration failed: Maximum clients reached\n");
    }

    pthread_mutex_unlock(&cr->mutex);

    return client;
}

int creg_unregister(CLIENT_REGISTRY *cr, CLIENT *client) {
    if (cr == NULL || client == NULL) return -1;

    // Lock the mutex before accessing shared data
    pthread_mutex_lock(&cr->mutex);

    // Find the client in the registry
    int index = -1;
    for (int i = 0; i < cr->client_count; i++) {
        if (cr->clients[i] == client) {
            index = i;
            break;
        }
    }

    // If found, remove the client from the registry
    if (index != -1) {
        cr->clients[index] = cr->clients[--cr->client_count];
        client_unref(client, "Unregistering client");
        // If this was the last client, release the semaphore
        if (cr->client_count == 0) {
            V(&cr->semaphore); // sem_post
            debug("Last client unregistered\n");
        }
    }

    pthread_mutex_unlock(&cr->mutex);

    if (index != -1) {
        return 0;
    }

    return -1;
}

CLIENT **creg_all_clients(CLIENT_REGISTRY *cr) {
    if (cr == NULL) return NULL;

    // Lock the mutex before accessing shared data
    pthread_mutex_lock(&cr->mutex);

    // Allocate memory for the array of clients
    CLIENT **client_list = malloc((cr->client_count + 1) * sizeof(CLIENT *));
    if (client_list == NULL) {
        pthread_mutex_unlock(&cr->mutex);
        return NULL;
    }

    // Copy the pointers to the array
    for (int i = 0; i < cr->client_count; i++) {
        client_list[i] = client_ref(cr->clients[i], "Retrieving client list");
    }

    // Add NULL terminator
    client_list[cr->client_count] = NULL;

    // Unlock the mutex
    pthread_mutex_unlock(&cr->mutex);

    debug("Retrieved client list\n");
    return client_list;
}

void creg_shutdown_all(CLIENT_REGISTRY *cr) {
    if (cr == NULL) return;

    // Lock the mutex before accessing shared data
    pthread_mutex_lock(&cr->mutex);

    // Shutdown all client connections
    for (int i = 0; i < cr->client_count; i++) {
        shutdown(client_get_fd(cr->clients[i]), SHUT_RDWR);
    }

    // Unlock the mutex
    pthread_mutex_unlock(&cr->mutex);

    // Wait until all clients are unregistered
    P(&cr->semaphore); // sem_wait
    debug("All clients shutdown\n");
}
