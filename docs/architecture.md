# System Architecture

## 1. Overview

This project implements a **minimal SIP communication system** consisting of:

* A **SIP Proxy Server** (Linux, UDP and TCP)
* A **SIP Client** (Windows/Linux, with Qt UI)

The system supports a basic SIP call lifecycle:

REGISTER → INVITE → CALL SETUP → BYE

The architecture is designed to demonstrate:

* Multithreaded server design
* Message-based processing
* Clear separation of concerns
* Safe memory management (RAII, smart pointers)
* Pluggable transport layer (UDP and TCP behind a shared interface)

---

## 2. High-Level Architecture

### 2.1 System Components

```
+----------------+         +----------------+
|   SIP Client A |         |   SIP Client B |
| (UI + Logic)   |         | (UI + Logic)   |
+--------+-------+         +--------+-------+
         |                          |
         |    UDP or TCP (SIP)      |
         +-----------+--------------+
                     |
              +------+------+
              |  SIP Proxy  |
              |   Server    |
              +-------------+
```

The proxy acts as an intermediary:

* Receives all SIP messages (over UDP or TCP)
* Parses and processes them
* Routes them to the correct destination using the same transport the client used

---

## 3. SIP Proxy Architecture

### 3.1 Processing Pipeline

```
UDP Listener  |  TCP Listener
      \              /
       \            /
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

* Listens on UDP port **5060** (default)
* Receives raw SIP messages using `recvfrom`
* Sends messages using `sendto`
* Runs in **non-blocking mode**

Key responsibility:

* Convert network input into raw string messages

---

#### 3.2.2 TCP Listener *(added)*

* Listens on a configurable TCP port (default **5061** when enabled)
* Accepts incoming TCP connections in a dedicated **accept thread**
* Each accepted connection gets its own **receive thread** that reads until `\r\n\r\n` (SIP message boundary)
* Sends responses back on the same persistent TCP connection

Framing:

* TCP is stream-based. Because all SIP messages in this system have an empty body (`Content-Length: 0`), every complete message ends with `\r\n\r\n`, which is used as the message delimiter.

Key responsibility:

* Provide the same `IUdpTransport` interface as the UDP listener so the rest of the proxy is transport-agnostic.

---

#### 3.2.3 Dispatcher Thread

* Single thread responsible for:

  * Receiving messages from the active transport (UDP or TCP)
  * Pushing them into a thread-safe queue

Why it exists:

* Decouples **network I/O** from **processing**
* Prevents blocking worker threads on I/O

---

#### 3.2.4 Concurrent Queue

* Thread-safe queue shared between dispatcher and workers
* Supports:

  * Blocking `pop()`
  * Safe shutdown

Implementation requirements:

* No busy waiting
* Uses `std::mutex` + `std::condition_variable`

---

#### 3.2.5 Worker Thread Pool

* Fixed number of worker threads
* Each thread:

  * Pulls messages from queue
  * Processes them independently

Benefits:

* Scales with number of concurrent requests
* Avoids creating threads per request

---

#### 3.2.6 SIP Parser & Serializer

Parser:

* Converts raw string → `SIPMessage`

Serializer:

* Converts `SIPMessage` → raw string

Important constraints:

* Header order must be preserved
* Duplicate headers allowed
* Content-Length must be accurate

---

#### 3.2.7 SIP Router

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

#### 3.2.8 Registration Table

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

#### 3.2.9 Call State Machine

Each call has a state:

```
INIT → TRYING → RINGING → ESTABLISHED → TERMINATED
```

Responsibilities:

* Track call progress
* Validate transitions
* Provide structured logging

---

#### 3.2.10 Structured Logger

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

## 4. Transport Abstraction

### 4.1 Interface — `IUdpTransport`

Both the UDP and TCP transports implement the same interface:

```cpp
class IUdpTransport {
public:
    using ReceiveCallback =
        std::function<void(const std::string& data,
                           const std::string& remote_ip,
                           uint16_t remote_port)>;

    virtual void start(uint16_t port, ReceiveCallback callback) = 0;
    virtual void send(const std::string& data,
                      const std::string& remote_ip,
                      uint16_t remote_port) = 0;
    virtual void stop() = 0;
};
```

The proxy and client hold a `std::unique_ptr<IUdpTransport>` and are completely unaware of which transport is active at runtime.

### 4.2 Concrete Implementations

| Class | Transport | Notes |
|-------|-----------|-------|
| `UdpTransport` | UDP | Single receive thread, `recvfrom`/`sendto` |
| `TcpTransport` | TCP | Accept thread + one receive thread per connection; `send()` looks up the correct fd by `ip:port` key |

### 4.3 TCP Transport — Modes of Operation

`TcpTransport` supports two modes:

**Server mode (proxy):** `start(port, cb)` binds and listens, then runs an accept loop. Each accepted client fd is stored in an internal map keyed by `"ip:port"` so that `send()` can find the right connection.

**Client mode (sip_client):** `connect(server_ip, server_port, cb)` opens a single connection to the proxy. `send()` writes on that single fd directly.

### 4.4 Selecting the Transport

**Proxy:**

```
./sip_proxy                          # UDP only (port 5060)
./sip_proxy --tcp-port 5061          # UDP + TCP (ports 5060 and 5061)
```

**Client:**

```
./sip_client 127.0.0.1 5060 --cli           # UDP
./sip_client 127.0.0.1 5061 --cli --tcp     # TCP
```

---

## 5. SIP Client Architecture

### 5.1 Processing Flow

```
Qt UI
    ↓
UI Controller
    ↓
SIP Message Builder
    ↓
Header Injector
    ↓
Transport (UdpTransport or TcpTransport)
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

### 5.2 Components Description

#### 5.2.1 Qt UI

Provides user interface for:

* Username input
* Proxy IP input
* Register button
* Call initiation
* Hangup
* Custom header input
* Transport selection (UDP/TCP)

Also displays:

* Call state
* Last SIP response

---

#### 5.2.2 UI Controller

* Connects UI actions to client logic
* Translates button clicks into SIP actions

---

#### 5.2.3 SIP Message Builder

Responsible for creating valid SIP messages:

* REGISTER
* INVITE
* ACK
* BYE

Ensures:

* Required headers are present
* Proper formatting

---

#### 5.2.4 Header Injection Engine

Allows dynamic modification of messages.

Supports:

* Adding headers
* Replacing headers

Rules:

* Add → append to end
* Replace → replace first occurrence

---

#### 5.2.5 Transport Layer

* Sends SIP messages to proxy via the active transport (`UdpTransport` or `TcpTransport`)
* Selected at startup via the `--tcp` flag; transparent to the rest of the client

---

#### 5.2.6 Response Listener Thread

* Dedicated thread listening for responses
* Processes incoming messages asynchronously

---

#### 5.2.7 Client State Machine

Tracks call state on client side:

* IDLE
* REGISTERED
* CALLING
* RINGING
* IN_CALL
* TERMINATED

Updates UI accordingly.

---

## 6. Concurrency Model

### Proxy

* 1 Dispatcher thread
* N Worker threads (thread pool)
* Shared queue
* UDP: 1 receive thread
* TCP: 1 accept thread + 1 receive thread per active connection

### Client

* 1 UI thread
* 1 Network listener thread (UDP receive loop or TCP receive loop)

---

### Key Rules

* No detached threads
* All threads must be joined on shutdown
* Use `atomic<bool>` for running flag
* Avoid race conditions using proper synchronization

---

## 7. Data Flow Example (INVITE over TCP)

1. Client A sends INVITE over TCP connection to proxy
2. Proxy's TCP receive thread reads message (framed by `\r\n\r\n`)
3. Dispatcher pushes to queue
4. Worker thread processes message
5. Parser creates SIPMessage
6. Router finds target (Client B) in registration table
7. Message forwarded to Client B via its TCP connection
8. Responses follow the same path back

---

## 8. Memory Management

Strict ownership model:

* `std::unique_ptr` → exclusive ownership (transport objects)
* `std::shared_ptr` → shared call session
* `std::weak_ptr` → avoid cycles

Rules:

* No raw `new/delete`
* RAII for all resources (sockets, threads, file descriptors)

---

## 9. Design Principles

### 9.1 Separation of Concerns

Each module has a single responsibility:

* Networking → transport only
* Proxy → SIP logic
* Client → UI + message generation

---

### 9.2 Message-Oriented Design

* System processes **messages**, not function calls
* Enables concurrency and scalability

---

### 9.3 Thread Safety First

* All shared data structures are protected
* No undefined behavior due to races

---

### 9.4 Pluggable Transport

* `IUdpTransport` decouples SIP logic from network I/O
* Switching between UDP and TCP requires no changes to the proxy or client business logic

---

### 9.5 Minimal but Extensible

System is intentionally simple, but allows:

* epoll-based networking
* TLS over TCP
* Advanced SIP features

---

## 10. Future Extensions

Possible improvements:

* epoll/reactor pattern
* TLS support
* Load testing tools
* Metrics endpoint
* SIP authentication

---

## 11. Summary

This architecture provides:

* Scalable request handling via thread pool
* Clear separation between networking and logic
* Safe concurrency model
* Pluggable transport layer (UDP and TCP behind `IUdpTransport`)
* Extensible SIP processing pipeline

It is designed for learning core backend concepts while remaining realistic enough to resemble production systems.
