gcc src/main.c src/blockchain/block.c src/blockchain/blockchain.c src/crypto/hash.c src/crypto/signature.c -o blockchain

blockchain.exe -- first and add

./blockchain

gcc src/viewer.c src/blockchain/block.c src/blockchain/blockchain.c src/crypto/hash.c src/crypto/signature.c -o viewer


viewer.exe

gcc src/validate.c src/blockchain/block.c src/crypto/hash.c -o validate_record

.\validate_record.exe


gcc -g \
src/test_node.c \
src/network/node.c \
src/network/protocol.c \
src/network/serializer.c \
src/network/proposal.c \
src/network/sync.c \
src/blockchain/blockchain.c \
src/blockchain/block.c \
src/crypto/hash.c \
src/crypto/signature.c \
-o node_app \
-lpthread -lcrypto


./node_app 8001 8002 8003
./node_app 8002 8001 8003
./node_app 8003 8001 8002

add record1.enc

for generating keys:
openssl genrsa -out keys/8001_private.pem 2048
openssl rsa -in keys/8001_private.pem -pubout -out keys/8001_public.pem


gcc src/generate_keys.c -o generate_keys -lcrypto

./generate_keys 8001 8002 8003


Added Commands

HEIGHT

LAST

PRINT <index>

VERIFY

PEERS

SYNC

HASH <file>

CHECKDUP <file>

CHECKSIG <index>

STATS

HELP


gcc test/benchmark_node.c \
src/network/node.c \
src/network/proposal.c \
src/network/protocol.c \
src/network/sync.c \
src/network/serializer.c \
src/blockchain/block.c \
src/blockchain/blockchain.c \
src/crypto/hash.c \
src/crypto/signature.c \
-lssl -lcrypto -lpthread \
-o benchmark_node
