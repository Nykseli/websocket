#ifndef WEB_SOCKET_BASE64_H
#define WEB_SOCKET_BASE64_H

int base64encode(const char *input, char *output, int len);
int base64decode(const char *input, char *output, int len);

#endif
