gcc src/main.c src/blockchain/block.c src/blockchain/blockchain.c src/crypto/hash.c src/crypto/signature.c -o blockchain

blockchain.exe -- first and add

./blockchain

gcc src/viewer.c src/blockchain/block.c src/blockchain/blockchain.c src/crypto/hash.c src/crypto/signature.c -o viewer


viewer.exe

gcc src/validate.c src/blockchain/block.c src/crypto/hash.c -o validate_record

.\validate_record.exe

