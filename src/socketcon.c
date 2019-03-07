
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

#include "dataframe.h"

// Check if the byte has mask
#define MASK_BIT(byte) (byte >> 7)

// Get the length bits
#define LENGTH_BITS(byte) (byte & 0x7f)

/**
 * @brief read the frame bytes from socket
 *
 * @param client client file descriptor
 * @return uint8_t pointer to data buffer
 */
static uint8_t* read_frame(int client) {
    // TODO: error handling. atleast the recv return handlinf
    // ptr tells us how many bytes have we read
    uint64_t ptr = 2;
    // frame needs atleast two bytes
    uint64_t frame_length = 2;
    uint64_t data_len = 0;
    // Allocate 2 bytes because the frame is atleast two bytes long
    uint8_t* buffer = malloc(sizeof(uint8_t) * frame_length);

    // read the first two bytes
    recv(client, buffer, sizeof(uint8_t) * 2, MSG_WAITALL);

    // If data length is represented in 16 bits
    if (LENGTH_BITS(buffer[1]) == 126) {
        frame_length += 2;
        buffer = realloc(buffer, sizeof(uint8_t) * frame_length);
        // Read the 16 bits of length to buffer
        recv(client, buffer + ptr, sizeof(uint8_t) * 2, 0);
        // Set the data length
        data_len += len_bytes_int(buffer + ptr, 2);
        ptr += 2;

    // If data length is represented in 64 bits
    } else if (LENGTH_BITS(buffer[1]) == 127) {
        frame_length += 8;
        buffer = realloc(buffer, sizeof(uint8_t) * frame_length);
        // Read the 64 bits of length to buffer
        recv(client, buffer + ptr, sizeof(uint8_t) * 8, 0);
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
        recv(client, buffer + ptr, sizeof(uint8_t) * 4, 0);
        // Move the pointer
        ptr += 4;
    }

    // Finally add the data_length to frame_length and allocate memory for it
    frame_length += data_len;
    buffer = realloc(buffer, sizeof(uint8_t) * frame_length);
    // Then read the rest of the frame i.e. the data bytes
    recv(client, buffer + ptr, sizeof(uint8_t) * data_len, 0);

    // Retun the pointer to the data
    return buffer;
}

static void close_socket(int client) {
    Dataframe frame;
    uint8_t* data_bytes;
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
    send(client, data_bytes, frame.total_len, 0);

    // Free the mallocs
    free_dataframe(&frame);
    free(data_bytes);
}

void handle_connection(int client) {
    // Connection loop
    // for (;;) {
    // accept 1 message for testing
    for (int i = 0; i < 1; i++) {
        Dataframe frame;
        uint8_t* frame_buf;
        // Create frame from the input data
        init_dataframe(&frame);
        frame_buf = read_frame(client);
        create_frame(&frame, frame_buf);
        free(frame_buf);

        // Send frame data verbatim to the server
        Dataframe echo;
        init_dataframe(&echo);
        set_as_last_frame(&echo);
        set_op_code(&echo, TEXT_FRAME);
        // Copy message data from recieved frame
        set_data(&echo, frame.data, frame.data_length);
        // Get bytes from frame and send it
        frame_buf = get_frame_bytes(&echo);
        send(client, frame_buf, echo.total_len, 0);

        // Remember to free the heap
        free(frame_buf);
        free_dataframe(&echo);
        free_dataframe(&frame);

    }

    // Send close frame for a clean close
    close_socket(client);
}
