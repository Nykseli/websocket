#ifndef WEB_SOCKET_SHA1_H
#define WEB_SOCKET_SHA1_H

#include <inttypes.h>

// Sha1 struct
// Sha1 hash contains always 20 bytes of data
typedef struct {
    uint8_t hash[20];
} Sha1;

/**
 * @brief Implementation of Sha-1 hashing algorithm
 *
 * See https://en.wikipedia.org/wiki/SHA-1 for more info
 *
 * @param sha pointer to Sha1 struct
 * @param input message bytes we want to has
 * @param msg_len amaount of bytes in message
 */
void sha1hash(Sha1* sha, const uint8_t *input, uint64_t msg_len);

#endif
