
#ifdef SHA1_TEST
#include <stdio.h>
#endif

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "sha1.h"

#ifdef SHA1_TEST
static void pbyte(uint8_t *bytes, int len) {
    for (int i = 0; i< len; i++) {
        printf("%02x ", bytes[i]);
    }
        printf("\n");
}
#endif

// https://en.wikipedia.org/wiki/Circular_shift
static uint32_t left_rotate(uint32_t value, unsigned int count) {
    return (value << count) | (value >> (32 - count));
}

static void copy_bytes(const uint8_t* from, uint8_t* to, uint64_t start, uint64_t end) {
    uint64_t index = 0;
    for (uint64_t i = start; i< end; i++){
        to[index] = from[i];
        index++;
    }
}

// Bytes is the buf[64] buffer
static void byte_to_uint32_big_endian(const uint8_t* bytes, uint32_t* ordered) {

    uint32_t num;
    for (int i = 0; i < 64; i += 4) {
        num  = 0;

        num |= (uint32_t) bytes[i] << 24;
        num |= (uint32_t) bytes[i + 1] << 16;
        num |= (uint32_t) bytes[i + 2] << 8;
        num |= (uint32_t) bytes[i + 3];

        ordered[i / 4] = num;
    }
}

// Translate the uint32_t to big-endian bytes
static void uint32_to_byte_big_endian(const uint32_t bits, uint8_t* ordered) {

    ordered[0] = (uint8_t)(bits>>24);
    ordered[1] = (uint8_t)(bits>>16);
    ordered[2] = (uint8_t)(bits>>8);
    ordered[3] = (uint8_t)bits;
}

// Translate the uint64_t to big-endian bytes
static void uint64_to_byte_big_endian(const uint64_t bits, uint8_t* ordered) {

    ordered[0] = (uint8_t)(bits>>54);
    ordered[1] = (uint8_t)(bits>>48);
    ordered[2] = (uint8_t)(bits>>40);
    ordered[3] = (uint8_t)(bits>>32);
    ordered[4] = (uint8_t)(bits>>24);
    ordered[5] = (uint8_t)(bits>>16);
    ordered[6] = (uint8_t)(bits>>8);
    ordered[7] = (uint8_t)bits;
}

/**
 * @brief Implementation of Sha-1 hashing algorithm
 *
 * See https://en.wikipedia.org/wiki/SHA-1 for more info
 *
 * @param sha pointer to Sha1 struct
 * @param input message bytes we want to has
 * @param msg_len amaount of bytes in message
 */
void sha1hash(Sha1* sha, const uint8_t *input, uint64_t msg_len){

    // Magic hash values
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    // Message length that is divisble by 64
    uint64_t message_64_len = msg_len + (64 -(msg_len % 64));
    // Allocate the memory for the new message
    uint8_t* message  = (uint8_t*) malloc(sizeof(uint8_t) * message_64_len);
    // Copy the input to message
    memcpy(message, input, msg_len);

    // Preprocess
    // Use msg_index as a help value to keep track where we want to append
    // Start from end of the input
    uint64_t msg_index = msg_len;
    // Append the '1' bit
    message[msg_index] = 0x80;
    msg_index++;
    // Append the '0' bits
    while (msg_index % 64 != 56) {
        message[msg_index] = 0;
        msg_index++;
    }

    // Append the message_len bits as big-endian
    uint8_t buf[8];
    // Message length in BITS not bytes
    uint64_to_byte_big_endian(msg_len * 8, buf);
    // Append the bits to message
    for (int i = 0; i < 8; i++) {
        message[msg_index] = buf[i];
        msg_index++;
    }

    // Hash each 512 bit (64 byte) chunk
    for(uint64_t i = 0; i<message_64_len; i += 64) {
        // Copy the chunk from message
        uint8_t int8buf[64];
        copy_bytes(message, int8buf, i, i + 64);

        uint32_t W32[80];
        // Get the first 16 bits from int8buf
        byte_to_uint32_big_endian(int8buf, W32);

        // Extend the sixteen 32-bit words into eighty 32-bit words
        for (int j = 16; j < 80; j++) {
            W32[j] = W32[j-3] ^ W32[j-8] ^ W32[j-14] ^ W32[j-16];
            W32[j] = left_rotate(W32[j], 1);
        }

        // Initialize hash values for this chunk
        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;
        // Other parameters that we need for hashing
        uint32_t f = 0;
        uint32_t k = 0;
        uint32_t tmp = 0;

        // Main loop
        for (int j = 0; j < 80; j++) {
            if(j < 20) {
                f = (b & c) | ((~b) & d);
                // Magic number
                k = 0x5a827999;
            } else if (j < 40) {
                f = b ^ c ^ d;
                // Magic number
                k = 0x6ed9eba1;
            } else if (j < 60) {
                f = (b & c) | (b & d) | (c & d);
                // Magic number
                k = 0x8f1bbcdc;
            } else {
                f = b ^ c ^ d;
                // Magic number
                k = 0xca62c1d6;
            }

            tmp = left_rotate(a, 5) + f + e + k + W32[j];
            e = d;
            d = c;
            c = left_rotate(b, 30);
            b = a;
            a = tmp;

        }

        // Add this chunks hash to result so far
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;

    }
    // Free the message
    free(message);

    uint8_t hbuf[4];
#ifdef SHA1_TEST
    // Print the hash
    uint32_to_byte_big_endian(h0, hbuf);
    for(int i = 0; i < 4; i++)  printf("%02x", hbuf[i]);
    uint32_to_byte_big_endian(h1, hbuf);
    for(int i = 0; i < 4; i++)  printf("%02x", hbuf[i]);
    uint32_to_byte_big_endian(h2, hbuf);
    for(int i = 0; i < 4; i++)  printf("%02x", hbuf[i]);
    uint32_to_byte_big_endian(h3, hbuf);
    for(int i = 0; i < 4; i++)  printf("%02x", hbuf[i]);
    uint32_to_byte_big_endian(h4, hbuf);
    for(int i = 0; i < 4; i++)  printf("%02x", hbuf[i]);
    printf("\n");
#endif

    // Finally copy the words to buffer
    uint32_to_byte_big_endian(h0, hbuf);
    memcpy(sha->hash, hbuf, 4);
    uint32_to_byte_big_endian(h1, hbuf);
    memcpy(sha->hash + 4, hbuf, 4);
    uint32_to_byte_big_endian(h2, hbuf);
    memcpy(sha->hash + 8, hbuf, 4);
    uint32_to_byte_big_endian(h3, hbuf);
    memcpy(sha->hash + 12, hbuf, 4);
    uint32_to_byte_big_endian(h4, hbuf);
    memcpy(sha->hash + 16, hbuf, 4);
}

#ifdef SHA1_TEST
int main(){
    Sha1 sha;
    sha1hash(&sha, (uint8_t*) "The quick brown fox jumps over the lazy cog", 43);
    pbyte(sha.hash, 20);
}
#endif
