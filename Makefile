CFLAGS := -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter

all: server client

server:
	gcc src/main.c -o server $(CFLAGS)

client:
	gcc src/test_client.c -o client $(CFLAGS)

base64:
	gcc src/crypto/base64.c -o base64 $(CFLAGS) -DBASE64_TEST

sha1:
	gcc src/crypto/sha1.c -o sha1 -g $(CFLAGS) -DSHA1_TEST

.PHONY: server base64 sha1
