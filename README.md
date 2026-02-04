# Custom Blockchain for Secure Medical Record Storage

This project implements a custom blockchain system designed specifically for the secure, integrity-verified storage of medical records. It utilizes a **Proof of Authority (PoA)** consensus mechanism and ensures data confidentiality through off-chain storage encryption.

## 🚀 Features

-   **Tamper-Proof Ledger**: Cryptographically verifiable history of all records.
-   **Proof of Authority Consensus**: Efficient validation without energy-intensive mining.
-   **Off-Chain Storage**: Secure, encrypted storage for sensitive medical files (`.enc`), keeping the lightweight chain fast.
-   **Distributed Network**: Support for multi-node validation and synchronization.
-   **Digital Signatures**: RSA-based signing for blocks and transactions.

## 📋 Prerequisites

To build and run this project, you need:

-   **GCC Compiler** (MinGW for Windows or standard GCC on Linux)
-   **OpenSSL** libraries (`libssl` and `libcrypto`)
-   **Pthread** library (usually included with GCC)

## 🛠️ Installation & Build

You can compile the different components of the system using the following commands.

### 1. Main Blockchain System
The core application for transaction creation and single-node operation.
```bash
gcc src/main.c src/blockchain/block.c src/blockchain/blockchain.c src/crypto/hash.c src/crypto/signature.c -o blockchain
```

### 2. Distributed Node Application
The networked version supporting multiple communicating nodes.
```bash
gcc -g src/test_node.c src/network/node.c src/network/protocol.c src/network/serializer.c src/network/proposal.c src/network/sync.c src/blockchain/blockchain.c src/blockchain/block.c src/crypto/hash.c src/crypto/signature.c -o node_app -lpthread -lcrypto
```

### 3. Blockchain Viewer
A read-only tool to explore the blockchain ledger.
```bash
gcc src/viewer.c src/blockchain/block.c src/blockchain/blockchain.c src/crypto/hash.c src/crypto/signature.c -o viewer
```

### 4. Record Validator
A standalone tool to verify the integrity of a medical record against the chain.
```bash
gcc src/validate.c src/blockchain/block.c src/crypto/hash.c -o validate_record
```

### 5. Key Generator
Utility to generate RSA key pairs for nodes.
```bash
gcc src/generate_keys.c -o generate_keys -lcrypto
```

### 6. Benchmark Tool
Test utility for performance benchmarking.
```bash
gcc test/benchmark_node.c src/network/node.c src/network/proposal.c src/network/protocol.c src/network/sync.c src/network/serializer.c src/blockchain/block.c src/blockchain/blockchain.c src/crypto/hash.c src/crypto/signature.c -lssl -lcrypto -lpthread -o benchmark_node
```

## 🖥️ Usage

### Running the Single Node Blockchain
```bash
./blockchain
```
This starts the CLI where you can add records and interact with the chain.

### Running a Distributed Network
To simulate a network, you can run multiple instances on different ports. First, ensure you have generated keys for them.

**Generate Keys:**
```bash
# Generate keys for nodes 8001, 8002, 8003
./generate_keys 8001 8002 8003
```
*Alternatively, using OpenSSL manually:*
```bash
openssl genrsa -out keys/8001_private.pem 2048
openssl rsa -in keys/8001_private.pem -pubout -out keys/8001_public.pem
```

**Run Nodes:**
Open separate terminals for each node:
```bash
# Node 1 (Port 8001, Peers: 8002, 8003)
./node_app 8001 8002 8003

# Node 2 (Port 8002, Peers: 8001, 8003)
./node_app 8002 8001 8003

# Node 3 (Port 8003, Peers: 8001, 8002)
./node_app 8003 8001 8002
```

### Blockchain Viewer
To inspect the current state of the blockchain:
```bash
./viewer
```

### validating a Record
To check if a record file has been tampered with:
```bash
./validate_record
```

## 🎮 Node Commands
When running the `node_app`, the following commands are available in the console:

- `HEIGHT`: Show the current block height.
- `LAST`: Display the last block's details.
- `PRINT <index>`: Print block details at a specific index.
- `VERIFY`: Run a full chain integrity verification.
- `PEERS`: List connected peer nodes.
- `SYNC`: Force a synchronization with peers.
- `HASH <file>`: Compute the hash of a specific file.
- `CHECKDUP <file>`: Check if a file already exists in the blockchain.
- `CHECKSIG <index>`: Verify the signature of a specific block.
- `STATS`: Show network and chain statistics.
- `HELP`: List all available commands.

## 📂 Project Structure

- `src/`: Source code for the application.
  - `blockchain/`: Core blockchain logic (blocks, chain management).
  - `crypto/`: Cryptographic functions (hashing, signatures).
  - `network/`: P2P networking and consensus logic.
- `offchain/`: Directory where encrypted medical records are stored.
- `keys/`: Storage for node public/private keys.
- `data/`: Persistent storage for local blockchain data.
- `test/`: Test scripts and benchmarks.
