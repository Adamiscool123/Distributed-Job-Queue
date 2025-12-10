# Distributed Job Queue System (DJQS)

A low-latency, horizontally scalable, and pure C++ job distribution system.  
Designed for asynchronous task handling across multiple Workers — with full thread safety.

---

## Overview

The Distributed Job Queue System (DJQS) solves a critical distributed-systems challenge:

> How do multiple Clients and Workers safely read/write the same queue at the same time?

The project demonstrates modern C++ concurrency design, safe shared-resource access, socket communication, and scalable worker orchestration.

---

## Architecture & Data Flow

DJQS follows the classic **Client → Server → Worker** flow.

- **Client** — Sends jobs  
- **Server** — Stores & dispatches jobs  
- **Workers** — Continuously pull & execute jobs  

---

## Components

| Component            | Role                                                  | Technologies                                  |
|----------------------|-------------------------------------------------------|-----------------------------------------------|
| **Server (Queue Boss)**   | Thread-safe job queue manager. Dispatches jobs to Workers. | Sockets, `std::thread`, `std::mutex`, `std::queue` |
| **Client (Request Maker)** | Sends tasks to the Server and disconnects after ACK.        | Sockets                                       |
| **Worker (Processor)**     | Pulls and processes tasks using simulated delays.           | Sockets, `sleep()`                            |

---

## Concurrency (The Real C++ Challenge)

The Server must prevent race conditions when multiple Workers try to pop from the queue.

### Problem

Two Workers grabbing the first item at the same time → **corrupted queue**, **crashes**, **undefined behavior**.

### Solution

Wrap every queue access inside a **mutex** lock.

---

## C++ Example

```cpp
🛠️ Installation
Prerequisites
GCC/G++ with C++11+

make (optional)

Compile Everything
bash
Copy code
g++ server.cpp -o server -std=c++17 -pthread
g++ client.cpp -o client -std=c++17 -pthread
g++ worker.cpp -o worker -std=c++17 -pthread
How to Run
Open multiple terminals:

Terminal	Command	Purpose
1	./server	Start the Server
2	./worker	Worker A
3	./worker	Worker B
4	./client	Send tasks

Sample Client Commands
Use these inside the Client terminal:

text
Copy code
TASK: CALCULATE_FINANCE: 5s_delay
TASK: CLEANUP_LOGS: 2s_delay
TASK: SEND_NOTIFICATION: 1s_delay
Workers will log the job they receive and sleep() to simulate real processing time.
