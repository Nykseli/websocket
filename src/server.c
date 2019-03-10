
/**
 * <sys/socket.h>
 *
 * defines:
 * PF_INET, SOCK_STREAM
 * AF_INET
 *
 * functions:
 * socket(), bind(), lisetn
 */
#include <sys/socket.h>

/**
 * <netinet/in.h>
 *
 * defines:
 * IPPROTO_TCP
 *
 * funcitons:
 * htons(), htonl()
 *
 * structs:
 * sockaddr_in
 */
#include <netinet/in.h>

/**
 * <stdint.h>
 *
 * structs:
 * uint16_t
 */
#include <stdint.h>

/**
 * <stdio.h>
 *
 * functions:
 * perror()
 */
#include <stdio.h>

/**
 * <stdlib.h>
 *
 * defines:
 * EXIT_FAILURE
 *
 * functions:
 * exit(), printf(), fgets(), feof(),
 * fclose()
 */
#include <stdlib.h>

/**
 * <string.h>
 *
 * functions:
 * memset(), strcpy()
 */
#include <string.h>
/**
 * <string.h>
 *
 * functions:
 * close(), read(), strlen()
 */
#include <unistd.h>

#include <pthread.h>

#include "http.h"
#include "server.h"

void init_server(WebSocketServer* wss) {
    wss->clients = NULL;
}

void free_server(WebSocketServer* wss) {
    if (wss->clients != NULL) {
        free(wss->clients);
    }
}

int run_server(WebSocketServer* wss) {

    // Ports range is 0-65535 (0x0000-0xffff)
    // which is exacly what uint16_t holds
    uint16_t port = 8888;

    pthread_t thread;

    struct sockaddr_in sa;

    // Set ipv4 tcp socket with stream socket
    int socketfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketfd == -1) {
        perror("cannot create socket");
        exit(EXIT_FAILURE);
    }

    // Try to reuse the port
    if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) == -1) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    // Initialize the sa struct with zeros (nulls) amount the struct size
    // TODO: do we really need this?
    memset(&sa, 0, sizeof(sa));

    // Set the socket addres to be ipv4
    sa.sin_family = AF_INET;
    // Set the socket port to port with correct byte order
    sa.sin_port = htons(port);
    // Set socket to accept any incoming messages
    sa.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the socketfd file descriptor to local addres (lsof -i tcp)
    if (bind(socketfd, (struct sockaddr*)&sa, sizeof(sa)) == -1) {
        perror("bind failed");
        close(socketfd);
        exit(EXIT_FAILURE);
    }

    // Prepare to accept connections on socket FD.
    // This tries 10 times if connection is working and returns -1
    // if there is a problem
    if (listen(socketfd, 10) == -1) {
        perror("listen failed");
        close(socketfd);
        exit(EXIT_FAILURE);
    }

    // Tcp connection loop
    for (;;) {
        // Wait for new connection and set the file descriptor for it
        int connectfd = accept(socketfd, NULL, NULL);

        // if file descriptor is -1, the kernel couldn't create new port
        // for the connection
        if (connectfd == -1) {
            perror("accept failed");
            close(socketfd);
            exit(EXIT_FAILURE);
        }

        // Handle each connection in a thread
        if (pthread_create(&thread, NULL, accept_client, &connectfd) != 0) {
            perror("thread failed");
            close(connectfd);
        }
    }


    close(socketfd);
    return EXIT_SUCCESS;
}
