CC=gcc
CFLAGS=-lssl -lcrypto

all:
	$(CC) src/main.c src/blockchain/blockchain.c src/crypto/hash.c src/crypto/signature.c src/cli/cli.c -o blockchain $(CFLAGS)
