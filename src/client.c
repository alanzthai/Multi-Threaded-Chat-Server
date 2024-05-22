#include <stdlib.h>
#include <stdio.h>
#include "csapp.h"
#include "client_registry.h"
#include "user_registry.h"
#include "globals.h"
#include "debug.h"

struct client {
    int fd; // File descriptor of the connection to the client
    USER *user; // Reference to the user object (if logged in)
    MAILBOX *mailbox; // Reference to the mailbox object (if logged in)
    pthread_mutex_t lock; // Mutex for thread safety
    CLIENT_REGISTRY *creg; // Reference to the client registry
    int ref_count;
};

// Internal function to send a packet to a client
int client_send_internal(CLIENT *client, CHLA_PACKET_HEADER *pkt, void *data) {
    // Check if client is logged in and has a valid file descriptor
    if (client->fd == -1 || client->user == NULL) {
        return -1;
    }

    // Send packet
    if(proto_send_packet(client->fd, pkt, data)) {
        return -1;
    }

    return 0;
}

// Create a new CLIENT object
CLIENT *client_create(CLIENT_REGISTRY *creg, int fd) {
    CLIENT *client = malloc(sizeof(CLIENT));
    if (!client) return NULL;

    client->fd = fd;
    client->user = NULL;
    client->mailbox = NULL;
    pthread_mutex_init(&(client->lock), NULL);
    client->creg = creg;
    client->ref_count = 1; // Initial reference count is 1

    return client;
}

// Increase the reference count on a CLIENT
CLIENT *client_ref(CLIENT *client, char *why) {
    pthread_mutex_lock(&(client->lock));
    client->ref_count++;
    debug("Client ref count: (%d -> %d)", client -> ref_count - 1,client -> ref_count);
    pthread_mutex_unlock(&(client->lock));
    return client;
}

// Decrease the reference count on a CLIENT
void client_unref(CLIENT *client, char *why) {
    pthread_mutex_lock(&(client->lock));
    client->ref_count--;
    debug("Client ref count: (%d -> %d)", client -> ref_count + 1,client -> ref_count);
    if (client->ref_count == 0) {
        // If reference count reaches 0, free the client object
        pthread_mutex_unlock(&(client->lock));
        pthread_mutex_destroy(&(client->lock));
        free(client);
    } else {
        pthread_mutex_unlock(&(client->lock));
    }
}

// Log this CLIENT in under a specified handle
int client_login(CLIENT *client, char *handle) {
    if (client->user != NULL) {
        // Client already logged in
        return -1;
    }

    // Register the user handle and create a new user object if necessary
    client->user = ureg_register(user_registry, handle);
    if (client->user == NULL) {
        // Failed to register user handle
        return -1;
    }

    // Create a new mailbox for the client
    client->mailbox = mb_init(handle);
    if (client->mailbox == NULL) {
        // Failed to create mailbox
        ureg_unregister(user_registry, handle); // Unregister user handle
        client->user = NULL;
        return -1;
    }

    // Login successful
    return 0;
}

// Log out this CLIENT
int client_logout(CLIENT *client) {
    if (client->user == NULL) {
        // Client not logged in
        return -1;
    }

    // Get the user handle
    char *handle = user_get_handle(client->user);

    // Unregister user handle and free resources
    user_unref(client->user, "Client logout");
    ureg_unregister(user_registry, handle);
    mb_shutdown(client->mailbox);
    mb_unref(client->mailbox, "Client logout");
    // client_unref(client, "Client logout");
    // mb_set_discard_hook(client->mailbox, NULL);


    // Update client state
    client->user = NULL;
    client->mailbox = NULL;

    return 0;
}

// Get the USER object for the specified logged-in CLIENT
USER *client_get_user(CLIENT *client, int no_ref) {
    if (client->user == NULL) {
        return NULL;
    }
    if (no_ref) {
        return client->user;
    }
    return user_ref(client->user, "Client get user");
}


// Get the MAILBOX for the specified logged-in CLIENT
MAILBOX *client_get_mailbox(CLIENT *client, int no_ref) {
    if (client->mailbox == NULL) {
        return NULL;
    }
    if (no_ref) {
        return client->mailbox;
    }
    mb_ref(client->mailbox, "Client get mailbox");
    return client->mailbox;
}


// Get the file descriptor for the network connection associated with this CLIENT
int client_get_fd(CLIENT *client) {
    return client->fd;
}

// Send a packet to a client
int client_send_packet(CLIENT *client, CHLA_PACKET_HEADER *pkt, void *data) {
    // Use client_send_internal to send packet
    pthread_mutex_lock(&(client->lock));
    if(client_send_internal(client, pkt, data)) {
        return -1;
    }
    pthread_mutex_unlock(&(client->lock));
    return 0;
}

// Send an ACK packet to a client
int client_send_ack(CLIENT *client, uint32_t msgid, void *data, size_t datalen) {
    // Create and send ACK packet
    CHLA_PACKET_HEADER pkt;
    memset(&pkt, 0, sizeof(CHLA_PACKET_HEADER));
    pkt.type = CHLA_ACK_PKT;
    pkt.msgid = htonl(msgid);
    pkt.payload_length = datalen;

    // Convert multi-byte fields in the header to network byte order
    // pkt.type = htons(pkt.type);
    // pkt.payload_length = htonl(pkt.payload_length);
    // pkt.msgid = htonl(pkt.msgid);

    // Thread safety
    // pthread_mutex_lock(&(client->lock));
    if(client_send_packet(client, &pkt, data)) {
        return -1;
    }
    // pthread_mutex_unlock(&(client->lock));
    return 0;
}

// Send an NACK packet to a client
int client_send_nack(CLIENT *client, uint32_t msgid) {
    // Create and send NACK packet
    CHLA_PACKET_HEADER pkt;
    memset(&pkt, 0, sizeof(CHLA_PACKET_HEADER));
    pkt.type = CHLA_NACK_PKT;
    pkt.msgid = htonl(msgid);
    pkt.payload_length = 0; // No payload for NACK

    // Thread safety
    // pthread_mutex_lock(&(client->lock));
    if(client_send_packet(client, &pkt, NULL)) {
        return -1;
    }
    // pthread_mutex_unlock(&(client->lock));
    return 0;
}

