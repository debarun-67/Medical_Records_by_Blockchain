---
trigger: always_on
---

# Design and Implementation of a Custom Blockchain for Secure Medical Record Storage

---

# 1. Project Overview

This project implements a **custom blockchain system** designed specifically for secure medical record storage.

The system ensures:

* Integrity of medical records
* Tamper detection
* Controlled trust using validator nodes
* Off-chain secure storage of sensitive data
* Auditability and traceability
* Multi-node distributed verification

The blockchain is implemented in C, with a modular cryptographic layer and Proof of Authority consensus.

---

# 2. Core Design Philosophy

The project follows five fundamental principles:

1. Medical data must not be publicly stored.
2. Integrity must be cryptographically verifiable.
3. Only authorized entities should validate records.
4. The system must scale to multiple servers.
5. Tampering must be detectable across the network.

---

# 3. System Architecture

## 3.1 High-Level Architecture

```
        ┌─────────────────────┐
        │   Hospital Client   │
        └─────────┬───────────┘
                  │
                  ▼
        ┌─────────────────────┐
        │  Application Layer  │
        │   (main.c logic)    │
        └─────────┬───────────┘
                  │
                  ▼
        ┌─────────────────────┐
        │  Blockchain Engine  │
        │ (blockchain.c)      │
        └─────────┬───────────┘
                  │
                  ▼
        ┌─────────────────────┐
        │   Validator Layer   │
        │ (signature.c)       │
        └─────────┬───────────┘
                  │
                  ▼
        ┌─────────────────────┐
        │ Persistent Ledger   │
        │ blockchain.dat      │
        └─────────────────────┘

Off-chain storage:
        offchain/records/*.enc
```

---

# 4. Data Flow

## 4.1 Record Insertion Flow

1. Doctor creates medical record.
2. Record stored in `offchain/records/recordX.enc`.
3. System generates cryptographic fingerprint.
4. Transaction created with:

   * Patient ID (pseudonymized)
   * Doctor ID
   * File pointer
   * File hash
5. Block created with:

   * Previous block hash
   * Timestamp
6. Block signed by validator (Proof of Authority).
7. Block appended to blockchain.dat.
8. Entire chain verified.

---

## 4.2 Record Validation Flow

1. Record file is selected.
2. Fingerprint is recomputed.
3. Blockchain is searched for matching transaction.
4. Stored hash compared to new hash.
5. If mismatch → tampering detected.

---

# 5. Current Single-Node Implementation

## Files Included

### main.c

* Entry point
* Creates blocks
* Handles record insertion
* Calls verification

### viewer.c

* Read-only blockchain explorer

### block.h / block.c

* Block structure
* Transaction structure
* Block initialization
* Transaction addition

### blockchain.h / blockchain.c

* Genesis creation
* Block persistence
* Chain verification
* Hash linkage validation

### hash.c

* SHA-256 implementation (single block)

### signature.c

* Validator signing (Proof of Authority)

### blockchain.dat

* Persistent ledger storage

---

# 6. Security Model

## 6.1 Integrity

Ensured by:

* SHA-256 fingerprint of medical record
* Hash chaining between blocks
* Validator signature over block hash

## 6.2 Confidentiality

Ensured by:

* Off-chain storage
* Encrypted `.enc` files

## 6.3 Authenticity

Ensured by:

* Validator signature
* Proof of Authority model

---

# 7. Multi-Node Scaling Plan (Full Distributed System)

To scale this system beyond a single machine, we introduce **multiple validator nodes**.

---

# 7.1 Distributed Network Design

```
             ┌──────────────┐
             │ Validator A  │
             └──────┬───────┘
                    │
                    ▼
             ┌──────────────┐
             │ Validator B  │
             └──────┬───────┘
                    │
                    ▼
             ┌──────────────┐
             │ Validator C  │
             └──────┬───────┘
                    │
                    ▼
             ┌──────────────┐
             │ Validator D  │
             └──────────────┘
```

Each node:

* Stores full blockchain copy
* Independently verifies new blocks
* Maintains identical ledger state

---

# 7.2 How Nodes Communicate

Future implementation will include:

* TCP socket layer in C
* Gossip protocol for block propagation
* Block broadcast mechanism
* Node synchronization on startup

---

# 7.3 Proof of Authority Consensus

Instead of mining:

1. A designated validator proposes block.
2. Other validators verify:

   * Previous hash
   * Transaction integrity
   * Signature
3. If majority agrees → block appended.

No nonce. No mining. No token.

---

# 8. Tamper Detection Across Multiple Servers

## Scenario 1: Off-chain file tampered on one server

1. Node A recomputes hash.
2. Hash mismatch detected.
3. Node A compares hash with:

   * Node B
   * Node C
4. Majority agrees original hash is correct.
5. Node A’s record marked as corrupted.

---

## Scenario 2: Blockchain file modified on one server

1. Hash linkage fails.
2. Validator signature verification fails.
3. Node requests chain copy from majority.
4. Tampered node restored.

---

## 8.1 Why Tampering Fails in Distributed Mode

An attacker would need to:

* Modify medical file
* Recompute hash
* Modify block
* Re-sign block
* Modify all subsequent blocks
* Compromise majority of validator nodes

This becomes computationally and administratively infeasible.

---

# 9. Server Scaling on Local System (Demonstration Plan)

To demonstrate distributed behavior on one system:

## Option 1: Multiple Executables

Create:

```
node1/
node2/
node3/
```

Each running:

```
blockchain_node.exe --port 8001
blockchain_node.exe --port 8002
blockchain_node.exe --port 8003
```

Each maintains:

```
nodeX/blockchain.dat
```

---

## Option 2: Docker Containers (Advanced)

Each container:

* Runs independent blockchain instance
* Synchronizes over local network

---

# 10. Future Enhancements (Full-Scale Implementation)

## 10.1 Full SHA-256 Multi-block Support

Replace minimal SHA implementation with:

* Streaming SHA-256
* OpenSSL library

## 10.2 AES Encryption for Medical Records

Encrypt `.enc` files with:

* AES-256
* Unique per-patient key

## 10.3 Role-Based Access Control

Add:

* Doctor nodes
* Patient nodes
* Regulatory auditor nodes

## 10.4 Smart Contracts (Solidity Layer)

For:

* Access permissions
* Automated authorization
* Consent management

---

# 11. Differences from Traditional Systems

| Feature              | Traditional Database | Custom Blockchain         |
| -------------------- | -------------------- | ------------------------- |
| Control              | Central admin        | Distributed validators    |
| Tamper logs          | Can be deleted       | Cryptographically chained |
| Single point failure | Yes                  | No                        |
| Trust                | Organizational       | Mathematical              |
| Transparency         | Controlled           | Auditable                 |

---

# 12. Full System Lifecycle

1. Record created.
2. Record encrypted.
3. Fingerprint generated.
4. Block created.
5. Validator signs block.
6. Block broadcast.
7. Majority validation.
8. Block appended on all nodes.
9. Future tampering checked by majority comparison.

---

# 13. Why This Design is Scalable

* Modular cryptographic layer
* Independent validator nodes
* Network-based propagation
* Stateless verification per block
* Off-chain storage for large data

---

# 14. Complete Project Scope (All Semesters)

### Semester 7 (Current Implementation)

* Single-node blockchain
* Proof of Authority
* Off-chain storage
* Tamper detection
* Blockchain viewer

### Semester 8 (Full Distributed System)

* Multi-node network
* TCP communication
* Consensus voting
* Auto-sync recovery
* Real encryption
* Access control layer

---

# 15. Final System Vision

A distributed, validator-based medical blockchain where:

* Every hospital runs a node
* Records are encrypted and stored locally
* Integrity proofs are shared network-wide
* Tampering is detected by cross-node validation
* Recovery is automatic via majority agreement
* No mining, no cryptocurrency, no unnecessary overhead

---

# 16. One-Line Technical Summary

A custom Proof-of-Authority blockchain designed for secure, scalable, and tamper-evident medical record storage with distributed validation and off-chain encrypted data management.

---
