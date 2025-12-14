# Distributed Job Queue System (DJQS)

A low-latency, horizontally scalable, and pure C++ job distribution system.  
Designed for asynchronous task handling across multiple Workers — with full thread safety.

---

## 📖 Overview

The Distributed Job Queue System (DJQS) solves a critical distributed-systems challenge:

> How do multiple Clients and Workers safely read/write the same queue at the same time?

The project demonstrates modern C++ concurrency design, safe shared-resource access, socket communication, and scalable worker orchestration.

---

## 🏗️ Architecture

DJQS follows the classic **Client → Server → Worker** flow.

```
┌─────────┐    ┌─────────┐    ┌─────────┐
│ Client  │ →  │ Server  │ →  │ Worker  │
└─────────┘    └─────────┘    └─────────┘
```

- **Client** — Sends jobs  
- **Server** — Stores & dispatches jobs  
- **Workers** — Continuously pull & execute jobs  

---

## 📦 Components

| Component | Role | Technologies Used |
|-----------|------|-------------------|
| **Server** | Thread-safe job queue manager. Dispatches jobs to Workers. | Sockets, `std::thread`, `std::mutex`, `std::queue` |
| **Client** | Sends tasks to the Server and disconnects after ACK. | Sockets |
| **Worker** | Pulls and processes tasks using simulated delays. | Sockets, `sleep()` |

---

## 🔐 Concurrency Solution

**Problem:** Two Workers grabbing the first item at the same time → corrupted queue, crashes, undefined behavior.

**Solution:** Wrap every queue access inside a **mutex** lock.

---

## 🚀 Quick Start

### Prerequisites
- GCC/G++ with C++11+
- `make` (optional)

### Compile Everything
```bash
g++ server.cpp -o server 
g++ client.cpp -o client 
g++ worker.cpp -o worker 
```

### Run the System
Open multiple terminals:

| Terminal | Command | Purpose |
|----------|---------|---------|
| 1 | `./server` | Start the Server |
| 2 | `./worker` | Worker A |
| 3 | `./worker` | Worker B |
| 4 | `./client` | Send tasks |

---

## 📋 Client Commands

### Command Arguments

| Argument | Description |
|----------|-------------|
| `--payload=` | The file you want to submit as data |
| `--priority=` | Job importance (0 = most important, 10 = least important) |
| `--retries=` | Number of retry attempts if job fails |
| `--deadline=` | Maximum time worker should take to complete job |

### Available Commands

#### Submit Commands
Convert video file (payload) to readable format for computers:
```bash
SUBMIT TRANSCODE_VIDEO --payload=video.mp4 --priority=2 --retries=3 --deadline=300
```

#### Monitoring & Control Commands
Get job status ("PENDING", "PROCESSING", "COMPLETED", "FAILED"):
```bash
JOB_STATUS <job_id>
```

Show system health:
```bash
METRICS_GET
```

Close client connection:
```bash
SHUTDOWN
```

## ✨ Features
- ✅ Thread-safe job queue
- ✅ Priority-based scheduling
- ✅ Automatic retry mechanism
- ✅ Deadline monitoring
- ✅ Multi-worker support
- ✅ Real-time job tracking

---

## 📚 Learning Points
- Distributed systems architecture
- C++ concurrency patterns
- Socket programming
- Race condition prevention
- Job scheduling algorithms

---

*Note: This is a simulation demonstrating distributed systems concepts on a single machine.*
