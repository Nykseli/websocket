CFLAGS := -std=c99 -Wall -Wextra -Werror -Wno-unused-parameter

all: server client

server:
	gcc src/main.c -o server $(CFLAGS)

client:
	gcc src/test_client.c -o client $(CFLAGS)

.PHONY: server
