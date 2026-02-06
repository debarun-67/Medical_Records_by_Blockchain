CC = gcc
CFLAGS = -Wall -Isrc
LIBS = -lssl -lcrypto

# Core modules source files
SRCS_COMMON = src/blockchain/block.c \
              src/blockchain/blockchain.c \
              src/crypto/hash.c \
              src/crypto/signature.c

# Targets
all: blockchain cli viewer validate keygen

blockchain: src/main.c $(SRCS_COMMON)
	$(CC) src/main.c $(SRCS_COMMON) -o blockchain $(CFLAGS) $(LIBS)

cli: src/cli/cli.c $(SRCS_COMMON)
	$(CC) src/cli/cli.c $(SRCS_COMMON) -o cli_tool $(CFLAGS) $(LIBS)

viewer: src/viewer.c $(SRCS_COMMON)
	$(CC) src/viewer.c $(SRCS_COMMON) -o viewer $(CFLAGS) $(LIBS)

validate: src/validate.c $(SRCS_COMMON)
	$(CC) src/validate.c $(SRCS_COMMON) -o validate_record $(CFLAGS) $(LIBS)

keygen: src/generate_keys.c
	$(CC) src/generate_keys.c -o generate_keys $(CFLAGS) $(LIBS)

clean:
	rm -f blockchain cli_tool viewer validate_record generate_keys
