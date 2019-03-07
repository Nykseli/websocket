#ifndef WEB_SOCKET_DATAFRAME_H
#define WEB_SOCKET_DATAFRAME_H

#include <inttypes.h>

/**
 *
 * Frame example
 * 0x81 0x05 0x48 0x65 0x6c 0x6c 0x6f (contains "Hello")
 *
 * 0x81 byte
 * FIN is 1 so this is the final frame, op code is 1 so this is text frame
 * 1000 0001
 *
 * 0x05 byte
 * Mask is 0, length is 5
 * 0000 0101
 * 0x48 0x65 0x6c 0x6c 0x6f Bytes are ascii chars
 * H    e    l    l    o
 *
 */

// Opcode enum contains the 4 bit part of the control byte
typedef enum {
    // Denotes a continuation frame
    CONT_FRAME = 0x0,
    // Denotes a text frame
    TEXT_FRAME = 0x1,
    // Denotes a binary frame
    BIN_FRAME  = 0x2,

    // 0x3-0x7 are reserved for further non-control frames

    // Denotes a connection close
    CLOSE_FRAME = 0x8,
    // Denotes a ping
    PING_FRAME = 0x9,
    // Denotes a pong
    PONG_FRAME = 0xa,

    // 0xb-0xf are reserved for further control frames

} Opcode;

// https://tools.ietf.org/html/rfc6455#section-5.2
typedef struct {
    // Control byte contains
    // FIN (bit), RSV1 (bit), RSV2 (bit), RSV2(bit), opcode(bibble)
    // in that order
    uint8_t control;
    // Data info contains
    // MASK (bit), Payload length (7 bits)
    // in that order
    uint8_t data_info;
    // This contains the length that is parsed from dataframe
    // accodring to data_info
    uint64_t data_length;
    // If the mask key is defined in data_info it's set here
    int32_t mask_key;
    // data contains the actual data being transfered
    // this needs to be allocated according to the data length
    // since it can be up to 2^64 - 1 bits
    uint8_t* data;
    // this is the total length of the bytes in the frame
    uint64_t total_len;

} Dataframe;

void init_dataframe(Dataframe *frame);
void free_dataframe(Dataframe *frame);
void set_as_last_frame(Dataframe *frame);
void set_op_code(Dataframe *frame, Opcode code);
void set_mask_key(Dataframe *frame, uint32_t mask_key);
void set_data(Dataframe *frame, uint8_t* data, uint64_t len);
uint64_t len_bytes_int(uint8_t* bytes, size_t size);
int create_frame(Dataframe *frame, uint8_t* data);
uint8_t* get_frame_bytes(Dataframe *frame);


#endif
