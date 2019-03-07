CFLAGS := -std=c99 -pthread -Wall -Wextra -Werror -Wno-unused-parameter

ifdef DEBUG
	CFLAGS += -g
endif

CRYPTOPATH = src/crypto/

all: server client

server: sha1.o base64.o dataframe.o socketcon.o
	gcc src/main.c -o server src/socketcon.o src/dataframe.o $(CRYPTOPATH)sha1.o $(CRYPTOPATH)base64.o $(CFLAGS)

client:
	gcc src/test_client.c -o client $(CFLAGS)

base64:
	gcc src/crypto/base64.c -o base64 $(CFLAGS) -DBASE64_TEST

sha1:
	gcc src/crypto/sha1.c -o sha1 -g $(CFLAGS) -DSHA1_TEST

sha1.o: $(CRYPTOPATH)sha1.c
	gcc $(CFLAGS) -c $(CRYPTOPATH)sha1.c -o $(CRYPTOPATH)sha1.o

base64.o: $(CRYPTOPATH)base64.c
	gcc $(CFLAGS) -c $(CRYPTOPATH)base64.c -o $(CRYPTOPATH)base64.o

dataframe.o: src/dataframe.c
	gcc $(CFLAGS) -c src/dataframe.c -o src/dataframe.o

socketcon.o: src/socketcon.c
	gcc $(CFLAGS) -c src/socketcon.c -o src/socketcon.o

.PHONY: server base64 sha1
