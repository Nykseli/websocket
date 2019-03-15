CFLAGS := -std=c99 -pthread -Wall -Wextra -Werror -Wno-unused-parameter

ifdef DEBUG
	CFLAGS += -g
endif

# Depencies for compiling
DEP_FILES=\
	sha1.o\
	base64.o\
	dataframe.o\
	socketcon.o\
	http.o\
	server.o

# Locations of object files
OBJ_FILES=\
	src/crypto/sha1.o\
	src/crypto/base64.o\
	src/dataframe.o\
	src/socketcon.o\
	src/http.o\
	src/server.o

CRYPTOPATH = src/crypto/

all: server client

server: $(DEP_FILES)
	gcc src/main.c -o server $(OBJ_FILES) $(CFLAGS)

client:
	gcc src/test_client.c -o client $(CFLAGS)

base64:
	gcc src/crypto/base64.c -o base64 $(CFLAGS) -DBASE64_TEST

sha1:
	gcc src/crypto/sha1.c -o sha1 -g $(CFLAGS) -DSHA1_TEST

sha1.o: $(CRYPTOPATH)sha1.c
	gcc $(CFLAGS) -fPIC -c $(CRYPTOPATH)sha1.c -o $(CRYPTOPATH)sha1.o

base64.o: $(CRYPTOPATH)base64.c
	gcc $(CFLAGS) -fPIC -c $(CRYPTOPATH)base64.c -o $(CRYPTOPATH)base64.o

dataframe.o: src/dataframe.c
	gcc $(CFLAGS) -fPIC -c src/dataframe.c -o src/dataframe.o

socketcon.o: src/socketcon.c
	gcc $(CFLAGS) -fPIC -c src/socketcon.c -o src/socketcon.o

http.o: src/http.c
	gcc $(CFLAGS) -fPIC -c src/http.c -o src/http.o

server.o: src/server.c
	gcc $(CFLAGS) -fPIC -c src/server.c -o src/server.o

shared: $(DEP_FILES)
	gcc $(OBJ_FILES) -shared -o libwebsocket.so $(CFLAGS)

# Install now moves the files to hardcoded paths
# TODO: fix this when creating configure
install:
	mkdir /usr/include/websocket
	cp src/server.h /usr/include/websocket/
	chmod 0644 libwebsocket.so
	mv libwebsocket.so /usr/lib/x86_64-linux-gnu/

uninstall:
	rm -r /usr/include/websocket
	rm /usr/lib/x86_64-linux-gnu/libwebsocket.so

.PHONY: server base64 sha1
