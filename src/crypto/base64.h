#ifndef WEB_SOCKET_BASE64_H
#define WEB_SOCKET_BASE64_H

#include <inttypes.h>

int base64encode(const uint8_t *input, char *output, int len);
int base64decode(const char *input, uint8_t *output, int len);

#endif
