

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

#define SERVER_STR "Server: webasmhttpd/0.0.1\r\n"

/**
 * @brief send 200 header to client
 *
 * @param client File descriptor
 * @param filename path and name to the file
 */
static void header_200(int client, const char *filename) {
    char buf[1024];

    //TODO: use filename to determine the Content-Type

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STR);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

/**
 * @brief send 404 header to client
 *
 * @param client File descriptor
 */
static void header_404(int client){
    char buf[1024];

    //TODO: use filename to determine the Content-Type

    strcpy(buf, "HTTP/1.0 404 Not Found\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STR);
    send(client, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

static void send_file_content(int client, FILE *resource) {
    char buf[1024];

    fgets(buf, sizeof(buf), resource);
    while (!feof(resource)) {
        send(client, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

static void sendFile(int client, const char* filename) {
    FILE *resource = NULL;

    resource = fopen(filename, "r");

    // Send 404 response if cannot find the file
    if(resource == NULL) {
        header_404(client);
    } else {
        header_200(client, filename);
        send_file_content(client, resource);
        // Finally close the file
        fclose(resource);
    }

}

int main(int argc, char const *argv[]) {

    // Ports range is 0-65535 (0x0000-0xffff)
    // which is exacly what uint16_t holds
    uint16_t port = 8888;

    // Buffer for reading data from client
    char buf[1024];

    struct sockaddr_in sa;

    // Set ipv4 tcp socket with stream socket
    int socketfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketfd == -1) {
        perror("cannot create socket");
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

        // Read the data from client and print it
        read(connectfd, buf, 1024);
        printf("Data from client:\n%s\n",  buf);

        // Serve the test html file to client
        sendFile(connectfd, "html/index_test.html");

        if (shutdown(connectfd, SHUT_RDWR) == -1) {
            perror("shutdown failed");
            close(connectfd);
            close(socketfd);
            exit(EXIT_FAILURE);
        }

        close(connectfd);
    }


    close(socketfd);
    return EXIT_SUCCESS;
}
