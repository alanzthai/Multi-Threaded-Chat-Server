#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "debug.h"
#include "server.h"
#include "globals.h"
#include "csapp.h"

static void terminate(int);

/*
 * "Charla" chat server.
 *
 * Usage: charla <port>
 */

// Function to handle SIGHUP signal
void sighup_handler(int signal) {
    terminate(EXIT_SUCCESS);
}

int main(int argc, char* argv[]){
    // Option processing should be performed here.
    // Option '-p <port>' is required in order to specify the port number
    // on which the server should listen.

    // Check if args are valid
    if (argv[1] && argv[2]){
        if (argc != 3 || strcmp(argv[1], "-p") != 0) {
            fprintf(stderr, "Invalid combination of args.\n");
            exit(EXIT_SUCCESS);
        }

        // Check if port number is valid
        char *endptr;
        long port = strtol(argv[2], &endptr, 10);
        if (*endptr != '\0' || port <= 0 || port > 65535 || errno == ERANGE) {
            fprintf(stderr, "Invalid port number.\n");
            exit(EXIT_SUCCESS);
        }
    }
    else{
        exit(EXIT_SUCCESS);
    }

    // Perform required initializations of the client_registry and
    // player_registry.
    user_registry = ureg_init();
    client_registry = creg_init();

    // TODO: Set up the server socket and enter a loop to accept connections
    // on this socket.  For each connection, a thread should be started to
    // run function charla_client_service().  In addition, you should install
    // a SIGHUP handler, so that receipt of SIGHUP will perform a clean
    // shutdown of the server.

    // Set up SIGHUP handler
    struct sigaction sa;
    sa.sa_handler = sighup_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGHUP, &sa, NULL) == -1) {
        fprintf(stderr, "Error installing SIGHUP handler");
        terminate(EXIT_FAILURE);
    }

    // Set up socket
    int *connfdp;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;
    int listenfd = open_listenfd(argv[2]);
    while (1) {
        clientlen = sizeof(struct sockaddr_storage);
        connfdp = Malloc(sizeof(int));
        *connfdp = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        Pthread_create(&tid, NULL, chla_client_service, connfdp);
    }


    terminate(EXIT_SUCCESS);
}

/*
 * Function called to cleanly shut down the server.
 */
static void terminate(int status) {
    // Shut down all existing client connections.
    // This will trigger the eventual termination of service threads.
    creg_shutdown_all(client_registry);

    // Finalize modules.
    creg_fini(client_registry);
    ureg_fini(user_registry);

    debug("%ld: Server terminating", pthread_self());
    exit(status);
}
