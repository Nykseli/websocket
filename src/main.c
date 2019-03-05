

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

#include "crypto/base64.h"
#include "crypto/sha1.h"
#include "dataframe.h"

#define SERVER_STR "Server: webasmhttpd/0.0.1\r\n"

static int read_line(int client, char *buffer, int size) {
    int i = 0, n;
    char c = 0;

    while(i < size && c != '\n') {
        n = recv(client, &c, 1, 0);

        if(n == -1) break; // TODO: Should we report an error?

        // ignore the \r
        if(c == '\r') continue;
        // when \n is found
        if(c == '\n') break;

        buffer[i] = c;
        i++;
    }

    // Terminate the string with null
    buffer[i] = '\0';

    return i;
}


static void get_str_from_buf(const char *s1, char *s2, int size, int start) {
    int char_start = 0;
    char c = s1[char_start + start];

    // Skip all the white space from the start
    while (c == ' ' || c == '\t') {
        c = s1[char_start + start];
        char_start++;
    }

    // Set rest of the chars to s2
    for(int i = 0; i < size; i++){
        c = s1[i + start + char_start];
        s2[i] = c;
    }
}

void sendFrame(int client) {
    Dataframe frame;
    init_dataframe(&frame);

    // We only want to test sending the one frame
    set_as_last_frame(&frame);

    // We want to send text data
    set_op_code(&frame, TEXT_FRAME);

    // Set the acutual message we want to send
    set_data(&frame, (uint8_t*)"Hello Sock!", 11);

    // Get the frame as a byte array
    uint8_t* data_bytes = get_data_bytes(&frame);

    // Send the actual data to client
    send(client, data_bytes, frame.total_len, 0);

    // Free the mallocs
    free_dataframe(&frame);
    free(data_bytes);

    // Send the close frame for a clean close

    // Re init the frame
    init_dataframe(&frame);

    // When sending a close frame. It has to be the last frame
    set_as_last_frame(&frame);

    // We want close the frame
    set_op_code(&frame, CLOSE_FRAME);

    // Set close message we want to send
    // 2 first bytes are the close code. and the rest is the message
    set_data(&frame, (uint8_t*)"\0\1Close Socket!", 15);

    // Get the frame as a byte array
    data_bytes = get_data_bytes(&frame);

    // Send the actual data to client
    send(client, data_bytes, frame.total_len, 0);

    // Free the mallocs
    free_dataframe(&frame);
    free(data_bytes);

}

static void socket_hash(char *in, char *out) {
    // Magic string for the socket handshake hash
    const char* magic_str = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char buf[256];
    Sha1 sha1;

    // Concat the input and the magic string
    strcpy(buf, in);
    strcpy(buf + strlen(in), magic_str);

    printf("BUF LENGTH: %s\n", buf);

    // Hask the concated string
    sha1hash(&sha1, (uint8_t*) buf, strlen(buf));

    // Base64 encode the hash to the out
    // sha hash is always 20 bytes long
    base64encode(sha1.hash, out, 20);

}

/**
 * @brief send 101 header to client. This is the webscoket handshake
 *
 * @param client File descriptor
 * @param accept the base64 string of the handshake hash
 */
static void header_101(int client, const char *accept) {
    char buf[1024];

    strcpy(buf, "HTTP/1.1 101 Switching Protocols\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "Upgrade: websocket\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "Connection: Upgrade\r\n");
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "Sec-WebSocket-Accept: ");
    strcat(buf, accept);
    strcat(buf, "\r\n");
    printf("%s", buf);
    send(client, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(client, buf, strlen(buf), 0);
}

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

static void handle_request_header(int client) {
    char buf[256];
    char tmp[256];

    while(read_line(client, buf, sizeof(buf)) > 1){
        if (!strncmp(buf, "Sec-WebSocket-Key: ", 19)){
            get_str_from_buf(buf, tmp, sizeof(tmp), 19);
            // Create the Sec-WebSocket-Accept: header hash
            socket_hash(tmp, buf);
            // Send the 101 header to complete the websocket handshake
            header_101(client, buf);
            // Send a test frame to client right after the hanshake is comlete
            sendFrame(client);
        }
        // If the connection is regular http. Return html that lets
        // the client know that this is webscoket server only
        if (!strcmp(buf, "Connection: keep-alive")) {
            sendFile(client, "html/ws_only.html");
        }

    }

}

int main(int argc, char const *argv[]) {

    // Ports range is 0-65535 (0x0000-0xffff)
    // which is exacly what uint16_t holds
    uint16_t port = 8888;

    // Buffer for reading data from client

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

        // Read the data from client and print it
        handle_request_header(connectfd);

        if (shutdown(connectfd, SHUT_RDWR) == -1) {
            perror("shutdown failed");
            close(connectfd);
            close(socketfd);
            exit(EXIT_FAILURE);
        }

        printf("request handled\n");
        close(connectfd);
    }


    close(socketfd);
    return EXIT_SUCCESS;
}
