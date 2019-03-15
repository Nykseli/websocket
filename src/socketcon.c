
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>

#include "dataframe.h"
#include "socketcon.h"

// Check if the byte has mask
#define MASK_BIT(byte) (byte >> 7)

// Get the length bits
#define LENGTH_BITS(byte) (byte & 0x7f)

// Get the op code from byte. The op code is the four rightmost bits
#define OP_CODE(byte) (byte & 0x0f)

/**
 * @brief read the frame bytes from socket
 *
 * @param client client file descriptor
 * @return uint8_t pointer to data buffer
 */
static uint8_t* read_frame(Connection *conn) {
    // TODO: error handling. atleast the recv return handlinf
    // ptr tells us how many bytes have we read
    uint64_t ptr = 2;
    // frame needs atleast two bytes
    uint64_t frame_length = 2;
    uint64_t data_len = 0;
    // Allocate 2 bytes because the frame is atleast two bytes long
    uint8_t* buffer = malloc(sizeof(uint8_t) * frame_length);

    // read the first two bytes

    recv(conn->conn_fd, buffer, sizeof(uint8_t) * 2, MSG_WAITALL);
    // If data length is represented in 16 bits
    if (LENGTH_BITS(buffer[1]) == 126) {
        frame_length += 2;
        buffer = realloc(buffer, sizeof(uint8_t) * frame_length);
        // Read the 16 bits of length to buffer
        recv(conn->conn_fd, buffer + ptr, sizeof(uint8_t) * 2, 0);
        // Set the data length
        data_len += len_bytes_int(buffer + ptr, 2);
        ptr += 2;

    // If data length is represented in 64 bits
    } else if (LENGTH_BITS(buffer[1]) == 127) {
        frame_length += 8;
        buffer = realloc(buffer, sizeof(uint8_t) * frame_length);
        // Read the 64 bits of length to buffer
        recv(conn->conn_fd, buffer + ptr, sizeof(uint8_t) * 8, 0);
        // Set the data length
        data_len = len_bytes_int(buffer + ptr, 8);
        ptr += 8;

    // Else the data is represented in the original 7 bits
    } else {
        data_len = LENGTH_BITS(buffer[1]);
    }

    // Read the mask bytes if there is any
    if (MASK_BIT(buffer[1])) {
        frame_length += 4;
        buffer = realloc(buffer, sizeof(uint8_t) * frame_length);
        // Read the four mask bytes
        recv(conn->conn_fd, buffer + ptr, sizeof(uint8_t) * 4, 0);
        // Move the pointer
        ptr += 4;
    }

    // Finally add the data_length to frame_length and allocate memory for it
    frame_length += data_len;
    buffer = realloc(buffer, sizeof(uint8_t) * frame_length);
    // Then read the rest of the frame i.e. the data bytes
    recv(conn->conn_fd, buffer + ptr, sizeof(uint8_t) * data_len, 0);

    // Retun the pointer to the data
    return buffer;
}

/**
 * @brief send the close signal to client
 *
 * @param client client file descriptor
 * @return int -1 to signal the end of the connection
 */
static int close_socket(Connection *conn) {
    Dataframe frame;
    uint8_t* data_bytes;
    // Init the frame
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
    // Return -1 to signal the end of the socket connection
    return -1;
}

/**
 * @brief echo the frame back to the client
 *
 * @param client client file descriptor
 * @param Dataframe pointer to frame you want to send
 * @return int 1 to signal that connection keeps on going
 */
static int echo_frame(Connection *conn, Dataframe *frame) {
    Dataframe echo;
    uint8_t* echo_buf;

    // Init the echo frame
    init_dataframe(&echo);

    // We currently want to frame only a one frame
    set_as_last_frame(&echo);

    // Only the text frame is supported
    set_op_code(&echo, TEXT_FRAME);

    // Copy message data from recieved frame
    set_data(&echo, frame->data, frame->data_length);

    // Get bytes from frame and send it
    echo_buf = get_frame_bytes(&echo);
    send(conn->conn_fd, echo_buf, echo.total_len, 0);

    // free the mallocs
    free(echo_buf);
    free_dataframe(&echo);

    return 1;
}

static int handle_frame(Connection *conn, Dataframe *frame) {
    // return -1 if we dont want to close the connection
    int return_val = 1;

    printf("Handle frame op code: %02x\n", OP_CODE(frame->control));

    switch (OP_CODE(frame->control)) {
        case CONT_FRAME:
            //TODO:
            break;
        case  TEXT_FRAME:
            return_val = echo_frame(conn, frame);
            break;
        case BIN_FRAME:
            //TODO:
            break;
        case CLOSE_FRAME:
            return_val = close_socket(conn);
            break;
        case PING_FRAME:
            //TODO:
            break;
        case PONG_FRAME:
            //TODO:
            break;
        default:
            //TODO: error maybe?
            break;
    }

    // Remember to free the frame after its used
    free_dataframe(frame);
    return return_val;
}

void handle_connection(Connection *conn) {
    // Connection loop
    // The connection thread dies after this loop breaks
    for (;;) {
        Dataframe frame;
        uint8_t* frame_buf;
        // Create frame from the input data
        init_dataframe(&frame);
        frame_buf = read_frame(conn);
        create_frame(&frame, frame_buf);
        // create_frame copies the frame struct so we need to free it
        free(frame_buf);

        // -1 from handle_frame signals the end of the connection
        if (handle_frame(conn, &frame) == -1)
            break;
    }
}

/**
 * @brief Close the socket and free the Connection struct
 *
 * @param conn Connection struct
 */
void close_connection(Connection* conn) {
    close(conn->conn_fd);
}
