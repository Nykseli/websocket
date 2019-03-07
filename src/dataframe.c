
#include <stdlib.h>
#include <string.h>
#include "dataframe.h"

// Check if the frame is the last one of the message
// FIN is the leftmost bit in the control byte
#define IS_LAST_FRAME(frame) ((frame)->control >> 7)

// Check if the mask flag is set
// Mask flag is the lefmost bit of the data_info byte
#define HAS_MASK(frame) ((frame)->data_info >> 7)

// Gets the length bytes from Dataframe struct
#define DATA_INFO_LEN(frame) ((frame)->data_info & 0x7f)

// Turns the frame->data_length into two byte array
#define UINT16_LEN_BYTES(frame)\
(uint8_t[])\
{\
    (uint8_t)((frame)->data_length >> 8),\
    (uint8_t)((frame)->data_length)\
}

// Turns the frame->data_length into eight byte array
#define UINT64_LEN_BYTES(frame)\
(uint8_t[])\
{\
    (uint8_t)((frame)->data_length >> 56),\
    (uint8_t)((frame)->data_length >> 48),\
    (uint8_t)((frame)->data_length >> 40),\
    (uint8_t)((frame)->data_length >> 32),\
    (uint8_t)((frame)->data_length >> 24),\
    (uint8_t)((frame)->data_length >> 16),\
    (uint8_t)((frame)->data_length >> 8),\
    (uint8_t)((frame)->data_length)\
}

// Turns the frame->mask_key into four byte array
#define MASK_BYTES(frame)\
(uint8_t[])\
{\
    (uint8_t)((frame)->mask_key >> 24),\
    (uint8_t)((frame)->mask_key >> 16),\
    (uint8_t)((frame)->mask_key >> 8),\
    (uint8_t)((frame)->mask_key)\
}


void init_dataframe(Dataframe *frame) {
    frame->control = 0;
    frame->data_info = 0;
    frame->data_length = 0;
    frame->mask_key = 0;
    frame->data = NULL;
    frame->total_len = 0;
}

void free_dataframe(Dataframe *frame) {
    free(frame->data);
}

/**
 * @brief Set the frame to be the last one of the message
 *
 * @param frame The Dataframe you want to set as last one
 */
void set_as_last_frame(Dataframe *frame) {
    // First bit of the control byte sets the last byte flag
    frame->control |= 0x80;
}

void set_RSV1(Dataframe *frame) {
    // TODO:
}

void set_RSV2(Dataframe *frame) {
    //TODO:
}

void set_RSV3(Dataframe *frame) {
    //TODO:
}

/**
 * @brief Set the opcode of the frame
 *
 * @param frame The Dataframe you want to set as last one
 * @param code Code from the Opcode enum. See dataframe.h
 */
void set_op_code(Dataframe *frame, Opcode code) {
    // Opcode is the 4 rightmost bits in control byte
    frame->control |= code;
}

/**
 * @brief Set the mask flag and set the mask_key value
 *
 * @param frame The Dataframe you want to set the mask to
 * @param mask_key uint32_t that contains the bits for the masking
 */
void set_mask_key(Dataframe *frame, uint32_t mask_key) {
    // Set the mask flag
    // Flag is the rightmost bit of the info byte
    frame->data_info |= 0x80;
    // And give the mask_key its value
    frame->mask_key = mask_key;
}

/**
 * @brief Allocate the memory for the actual data and set it
 *
 * @param frame The Dataframe you want to set the data to
 * @param mask_key byte array of the data you want to set
 * @param len the length of the array
 */
void set_data(Dataframe *frame, uint8_t* data, uint64_t len) {
    // First set the data_info length bytes
    // Length bits are the seven left most bits
    // if length is < 126 we can store the whole length to info_byte
    if (len < 126) {
        frame->data_info |= (uint8_t)len;
    } else if (len <= UINT16_MAX) {
        // If the length is in range of UINT16_MAX (0-65535)
        // we need to set the lenght of the frame data_info to 126 (111 1110)
        frame->data_info |= 126;
    } else {
        // If the length is higher than UINT16_MAX (0-65535)
        // we need to set the lenght of the frame data_info to 127 (111 1111)
        frame->data_info |= 127;
    }

    // Set the data_length
    frame->data_length = len;

    // Allocate the memory for the data and copy the data to it
    frame->data = (uint8_t*) malloc(sizeof(uint8_t) * len);
    memcpy(frame->data, data, len);

}

//TODO: create frame from byte array
/**
 * @brief Get the byte array in from the frame struct
 *
 * @param frame The Dataframe you want to set the data to
 * @return uint8_t* pointer to the array
 */
uint8_t* get_data_bytes(Dataframe *frame) {
    uint8_t* data_bytes = NULL;
    // Extra bytes are the bytes that are not the actual data bytes
    // We always need atleast two since for the control and info byte
    uint64_t extra_bytes = 2;

    // If the length in data_info is 126, we need two bytes to represent the value
    // 126 reprecent 16 bit length value
    if (DATA_INFO_LEN(frame) == 126)
        extra_bytes += 2;
    // If the length in data_info is 127, we need eight bytes to represent the value
    // 127 reprecent 64 bit length value
    else if (DATA_INFO_LEN(frame) == 127)
        extra_bytes += 8;

    // If frame has  mask, we need four bytes to store it
    if (HAS_MASK(frame))
        extra_bytes += 4;

    // The total of the frame is the length of the data bytes and the extra bytes
    uint64_t length = frame->data_length + extra_bytes;
    // Set the total length to frame struct so it can be used when sending the data
    frame->total_len = length;
    // Allocate the memory for the byte array
    data_bytes = (uint8_t*) malloc(sizeof(uint8_t) * length);

    // The control byte needs to be first
    data_bytes[0] = frame->control;
    // The second contains the frame info
    data_bytes[1] = frame->data_info;

    // If the info length is 126 copy the full length as two byte array
    if (DATA_INFO_LEN(frame) == 126)
        memcpy(data_bytes + 2, UINT16_LEN_BYTES(frame), 2);
    // If the info length is 127 copy the full length as eight byte array
    else if (DATA_INFO_LEN(frame) == 127)
        memcpy(data_bytes + 2, UINT64_LEN_BYTES(frame), 8);

    // If the frame has mask, append the four mask bytes to end of data_bytes
    if (HAS_MASK(frame))
        memcpy(data_bytes + strlen((char*)data_bytes), MASK_BYTES(frame), 4);

    // Copy the actual data from frame to array
    memcpy(data_bytes + extra_bytes, frame->data, frame->data_length);

    return data_bytes;
}
