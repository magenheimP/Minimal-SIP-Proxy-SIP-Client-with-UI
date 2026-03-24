# System Architecture

## 1. Overview

This project implements a **minimal SIP communication system** consisting of:

* A **SIP Proxy Server** (Linux, UDP-based)
* A **SIP Client** (Windows, with Qt UI)

The system supports a basic SIP call lifecycle:

REGISTER → INVITE → CALL SETUP → BYE

The architecture is designed to demonstrate:

* Multithreaded server design
* Message-based processing
* Clear separation of concerns
* Safe memory management (RAII, smart pointers)

---

## 2. High-Level Architecture

### 2.1 System Components

```
+----------------+         +----------------+
|   SIP Client A |         |   SIP Client B |
| (UI + Logic)   |         | (UI + Logic)   |
+--------+-------+         +--------+-------+
         |                          |
         |        UDP (SIP)         |
         +-----------+--------------+
                     |
              +------+------+
              |  SIP Proxy  |
              |   Server    |
              +-------------+
```

The proxy acts as an intermediary:

* Receives all SIP messages
* Parses and processes them
* Routes them to the correct destination

---

## 3. SIP Proxy Architecture

### 3.1 Processing Pipeline

```
UDP Listener
    ↓
Dispatcher Thread
    ↓
Concurrent Queue<SIPMessage>
    ↓
Worker Thread Pool
    ↓
SIP Router
    ↓
Call State Machine
    ↓
Logger
```

---

### 3.2 Components Description

#### 3.2.1 UDP Listener

* Listens on UDP port **5060**
* Receives raw SIP messages using `recvfrom`
* Sends messages using `sendto`
* Runs in **non-blocking mode**

Key responsibility:

* Convert network input into raw string messages

---

#### 3.2.2 Dispatcher Thread

* Single thread responsible for:

    * Receiving messages from UDP socket
    * Pushing them into a thread-safe queue

Why it exists:

* Decouples **network I/O** from **processing**
* Prevents blocking worker threads on I/O

---

#### 3.2.3 Concurrent Queue

* Thread-safe queue shared between dispatcher and workers
* Supports:

    * Blocking `pop()`
    * Safe shutdown

Implementation requirements:

* No busy waiting
* Uses `std::mutex` + `std::condition_variable`

---

#### 3.2.4 Worker Thread Pool

* Fixed number of worker threads
* Each thread:

    * Pulls messages from queue
    * Processes them independently

Benefits:

* Scales with number of concurrent requests
* Avoids creating threads per request

---

#### 3.2.5 SIP Parser & Serializer

Parser:

* Converts raw string → `SIPMessage`

Serializer:

* Converts `SIPMessage` → raw string

Important constraints:

* Header order must be preserved
* Duplicate headers allowed
* Content-Length must be accurate

---

#### 3.2.6 SIP Router

Core logic of the proxy.

Responsibilities:

* Identify message type (REGISTER, INVITE, response)
* Route messages to correct destination
* Interact with registration table

Examples:

* REGISTER → store user location
* INVITE → forward to callee
* Responses → route back to caller

---

#### 3.2.7 Registration Table

* Thread-safe mapping:

```
User → (IP, Port)
```

Used for:

* Locating users during call setup

Requirements:

* Concurrent access safe
* Fast lookup

---

#### 3.2.8 Call State Machine

Each call has a state:

```
INIT → TRYING → RINGING → ESTABLISHED → TERMINATED
```

Responsibilities:

* Track call progress
* Validate transitions
* Provide structured logging

---

#### 3.2.9 Structured Logger

Logs every important event in format:

```
[Timestamp] [ThreadID] [Component] [CallID] [State] Message
```

Also logs:

* Custom headers:

  ```
  [CUSTOM HEADER] X-Debug-ID: 123
  ```

* Modified headers:

  ```
  [HEADER MODIFIED] Via: ...
  ```

---

## 4. SIP Client Architecture

### 4.1 Processing Flow

```
Qt UI
    ↓
UI Controller
    ↓
SIP Message Builder
    ↓
Header Injector
    ↓
UDP Sender
    ↓
Proxy
    ↓
Response Listener Thread
    ↓
Client State Machine
    ↓
UI Update
```

---

### 4.2 Components Description

#### 4.2.1 Qt UI

Provides user interface for:

* Username input
* Proxy IP input
* Register button
* Call initiation
* Hangup
* Custom header input

Also displays:

* Call state
* Last SIP response

---

#### 4.2.2 UI Controller

* Connects UI actions to client logic
* Translates button clicks into SIP actions

---

#### 4.2.3 SIP Message Builder

Responsible for creating valid SIP messages:

* REGISTER
* INVITE
* ACK
* BYE

Ensures:

* Required headers are present
* Proper formatting

---

#### 4.2.4 Header Injection Engine

Allows dynamic modification of messages.

Supports:

* Adding headers
* Replacing headers

Rules:

* Add → append to end
* Replace → replace first occurrence

---

#### 4.2.5 UDP Sender

* Sends SIP messages to proxy via UDP
* Uses same transport abstraction as proxy

---

#### 4.2.6 Response Listener Thread

* Dedicated thread listening for responses
* Processes incoming messages asynchronously

---

#### 4.2.7 Client State Machine

Tracks call state on client side:

* IDLE
* REGISTERED
* CALLING
* RINGING
* IN_CALL
* TERMINATED

Updates UI accordingly.

---

## 5. Concurrency Model

### Proxy

* 1 Dispatcher thread
* N Worker threads (thread pool)
* Shared queue

### Client

* 1 UI thread
* 1 Network listener thread

---

### Key Rules

* No detached threads
* All threads must be joined on shutdown
* Use `atomic<bool>` for running flag
* Avoid race conditions using proper synchronization

---

## 6. Data Flow Example (INVITE)

1. Client A sends INVITE
2. Proxy receives via UDP
3. Dispatcher pushes to queue
4. Worker thread processes message
5. Parser creates SIPMessage
6. Router finds target (Client B)
7. Message forwarded to Client B
8. Responses follow same path back

---

## 7. Memory Management

Strict ownership model:

* `std::unique_ptr` → exclusive ownership
* `std::shared_ptr` → shared call session
* `std::weak_ptr` → avoid cycles

Rules:

* No raw `new/delete`
* RAII for all resources (sockets, threads)

---

## 8. Design Principles

### 8.1 Separation of Concerns

Each module has a single responsibility:

* Networking → transport only
* Proxy → SIP logic
* Client → UI + message generation

---

### 8.2 Message-Oriented Design

* System processes **messages**, not function calls
* Enables concurrency and scalability

---

### 8.3 Thread Safety First

* All shared data structures are protected
* No undefined behavior due to races

---

### 8.4 Minimal but Extensible

System is intentionally simple, but allows:

* epoll-based networking
* TCP transport
* Advanced SIP features

---

## 9. Future Extensions

Possible improvements:

* epoll/reactor pattern
* TCP support
* Load testing tools
* Metrics endpoint
* SIP authentication

---

## 10. Summary

This architecture provides:

* Scalable request handling via thread pool
* Clear separation between networking and logic
* Safe concurrency model
* Extensible SIP processing pipeline

It is designed for learning core backend concepts while remaining realistic enough to resemble production systems.
