
/**
 * <netinet/in.h>
 *
 * defines:
 * IPPROTO_TCP
 *
 * funcitons:
 * htons(), htonl(), send()
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
 * memset(), strcpy(), strlen()
 */
#include <string.h>
/**
 * <string.h>
 *
 * functions:
 * close(), read()
 */
#include <unistd.h>

#include "crypto/base64.h"
#include "crypto/sha1.h"
#include "dataframe.h"
#include "socketcon.h"

#define SERVER_STR "Server: webasmhttpd/0.0.1\r\n"

static int read_line(Connection *conn, char *buffer, int size) {
    int i = 0, n;
    char c = 0;

    while(i < size && c != '\n') {
        n = recv(conn->conn_fd, &c, 1, 0);

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

void sendFrame(Connection *conn) {
    Dataframe frame;
    init_dataframe(&frame);

    // We only want to test sending the one frame
    set_as_last_frame(&frame);

    // We want to send text data
    set_op_code(&frame, TEXT_FRAME);

    // Set the acutual message we want to send
    set_data(&frame, (uint8_t*)"Hello Sock!", 11);

    // Get the frame as a byte array
    uint8_t* data_bytes = get_frame_bytes(&frame);

    // Send the actual data to client
    send(conn->conn_fd, data_bytes, frame.total_len, 0);

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
    data_bytes = get_frame_bytes(&frame);

    // Send the actual data to client
    send(conn->conn_fd, data_bytes, frame.total_len, 0);

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
 * @param conn Connection struct
 * @param accept the base64 string of the handshake hash
 */
static void header_101(Connection *conn, const char *accept) {
    char buf[1024];

    strcpy(buf, "HTTP/1.1 101 Switching Protocols\r\n");
    send(conn->conn_fd, buf, strlen(buf), 0);
    strcpy(buf, "Upgrade: websocket\r\n");
    send(conn->conn_fd, buf, strlen(buf), 0);
    strcpy(buf, "Connection: Upgrade\r\n");
    send(conn->conn_fd, buf, strlen(buf), 0);
    strcpy(buf, "Sec-WebSocket-Accept: ");
    strcat(buf, accept);
    strcat(buf, "\r\n");
    printf("%s", buf);
    send(conn->conn_fd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(conn->conn_fd, buf, strlen(buf), 0);
}

/**
 * @brief send 200 header to client
 *
 * @param conn Connection struct
 * @param filename path and name to the file
 */
static void header_200(Connection *conn, const char *filename) {
    char buf[1024];

    //TODO: use filename to determine the Content-Type

    strcpy(buf, "HTTP/1.0 200 OK\r\n");
    send(conn->conn_fd, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STR);
    send(conn->conn_fd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(conn->conn_fd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(conn->conn_fd, buf, strlen(buf), 0);
}

/**
 * @brief send 404 header to client
 *
 * @param conn Connection struct
 */
static void header_404(Connection *conn){
    char buf[1024];

    //TODO: use filename to determine the Content-Type

    strcpy(buf, "HTTP/1.0 404 Not Found\r\n");
    send(conn->conn_fd, buf, strlen(buf), 0);
    strcpy(buf, SERVER_STR);
    send(conn->conn_fd, buf, strlen(buf), 0);
    sprintf(buf, "Content-Type: text/html\r\n");
    send(conn->conn_fd, buf, strlen(buf), 0);
    strcpy(buf, "\r\n");
    send(conn->conn_fd, buf, strlen(buf), 0);
}

static void send_file_content(Connection *conn, FILE *resource) {
    char buf[1024];

    fgets(buf, sizeof(buf), resource);
    while (!feof(resource)) {
        send(conn->conn_fd, buf, strlen(buf), 0);
        fgets(buf, sizeof(buf), resource);
    }
}

static void sendFile(Connection *conn, const char* filename) {
    FILE *resource = NULL;

    resource = fopen(filename, "r");

    // Send 404 response if cannot find the file
    if(resource == NULL) {
        header_404(conn);
    } else {
        header_200(conn, filename);
        send_file_content(conn, resource);
        // Finally close the file
        fclose(resource);
    }

}

//TODO: handle headers properly
static void handle_request_header(Connection *conn) {
    char buf[256];
    char tmp[256] = {0};

    while(read_line(conn, buf, sizeof(buf)) > 1){
        // If the web socket key is found, save the key to tmp and
        // read the rest of the header
        if (!strncmp(buf, "Sec-WebSocket-Key: ", 19)){
            get_str_from_buf(buf, tmp, sizeof(tmp), 19);
        }
        // If the connection is regular http. Return html that lets
        // the client know that this is webscoket server only
        if (!strcmp(buf, "Connection: keep-alive")) {
            sendFile(conn, "html/ws_only.html");
        }

    }

    // If the websocket key is found
    if (tmp[0] != 0) {
        // Create the Sec-WebSocket-Accept: header hash
        socket_hash(tmp, buf);
        // Send the 101 header to complete the websocket handshake
        header_101(conn, buf);
        // handle the web socket connection
        handle_connection(conn);
    }
}

void* accept_client(void* clientptr) {
        Connection conn;
        conn.conn_fd = *((int*) clientptr);

        // Read the data from client and print it
        handle_request_header(&conn);

        // if (shutdown(client, SHUT_RDWR) == -1) {
        //     perror("shutdown failed");
        //     close(client);
        //     exit(EXIT_FAILURE);
        // }

        printf("request handled\n");
        close_connection(&conn);

        return NULL;
}
