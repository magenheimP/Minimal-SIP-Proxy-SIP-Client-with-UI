# Interface Contracts — Minimal SIP Proxy & SIP Client

> **Project:** Minimal-SIP-Proxy-SIP-Client-with-UI  
> **Last updated:** 2026-03-24  
> **Source of truth:** `networking/`, `common/`, `proxy/`, `client/`, `ui/`

---

## Table of Contents

1. [Contract 1 — Networking → Proxy (Transport Abstraction)](#contract-1--networking--proxy-transport-abstraction)
2. [Contract 2 — Proxy Parser → Router (Shared SIPMessage Model)](#contract-2--proxy-parser--router-shared-sipmessage-model)
3. [Contract 3 — Proxy ↔ Client (SIP Message Requirements)](#contract-3--proxy--client-sip-message-requirements)
4. [Contract 4 — Header Injection Engine](#contract-4--header-injection-engine)
5. [Contract 5 — Logging Format](#contract-5--logging-format)
6. [Appendix A — Call State Machine](#appendix-a--call-state-machine)
7. [Appendix B — File Locations Quick Reference](#appendix-b--file-locations-quick-reference)

---

## Contract 1 — Networking → Proxy (Transport Abstraction)

**Owner:** Group 1 (Networking)  
**Header:** `networking/include/networking/IUdpTransport.hpp`  
**Concrete impl:** `networking/include/networking/udp_transport.hpp`

### Interface

```cpp
class IUdpTransport {
public:
    using ReceiveCallback =
        std::function<void(const std::string& data,
                           const std::string& remote_ip,
                           uint16_t remote_port)>;

    virtual ~IUdpTransport() = default;

    virtual void start(uint16_t port, ReceiveCallback callback) = 0;

    virtual void send(const std::string& data,
                      const std::string& remote_ip,
                      uint16_t remote_port) = 0;

    virtual void stop() = 0;
};
```

### Concrete Implementation — `UdpTransport`

`UdpTransport` extends `IUdpTransport` and adds:

```cpp
uint16_t local_port() const;   // query which port was bound
```

Internally it owns a `UdpSocket` and a receive thread (`std::thread recv_thread`). The receive loop runs while `std::atomic<bool> running` is `true`.

### Rules

| Rule | Description |
|------|-------------|
| **No SIP parsing** | The transport layer MUST NOT inspect or parse SIP content. |
| **No SIP ownership** | The transport layer MUST NOT own or store `SIPMessage` objects. |
| **Raw strings only** | Data is passed as a raw `std::string`. Parsing is the proxy's responsibility. |
| **Thread safety** | `send()` and `stop()` may be called from any thread; `UdpTransport` must handle concurrent access. |
| **Lifecycle** | Call `start()` exactly once before `send()`. Call `stop()` to terminate the receive loop cleanly. |

### Data Flow

```
Network (UDP) ──► UdpSocket ──► recv_thread ──► ReceiveCallback(data, ip, port)
                                                        │
                                                        ▼
                                                  SIPProxy::handle_packet()
                                                  (parsing happens here)
```

---

## Contract 2 — Proxy Parser → Router (Shared SIPMessage Model)

**Owner:** Group 2 (Proxy)  
**Header:** `common/include/common/sip_message.hpp`  
**Parser:** `proxy/include/proxy/sip_parser.hpp`  
**Router:** `proxy/include/proxy/sip_router.hpp`

### Shared Data Structures

```cpp
namespace common {

    struct SIPHeader {
        std::string name;
        std::string value;
    };

    struct SIPMessage {
        std::string            start_line;
        std::vector<SIPHeader> headers;
        std::string            body;

        // --- Accessors ---
        std::string              get_header(const std::string& name) const;
        std::vector<std::string> get_headers(const std::string& name) const;
        bool                     has_header(const std::string& name) const;

        // --- Mutators ---
        void set_header(const std::string& name, const std::string& value);
        void add_header(const std::string& name, const std::string& value);
        void prepend_header(const std::string& name, const std::string& value);
        bool remove_first_header(const std::string& name);
        void update_content_length();

        // --- Introspection ---
        bool        is_request()    const;
        bool        is_response()   const;
        std::string get_method()    const;
        int         get_status_code() const;

        // --- Serialization ---
        std::string serialize() const;
    };

}
```

### Parser Interface

```cpp
namespace proxy {

class SIPParser {
public:
    static common::SIPMessage parse(const std::string& raw_message);
private:
    static void parse_start_line(common::SIPMessage&, const std::string& line);
    static void parse_header_line(common::SIPMessage&, const std::string& line);
    static void parse_body(common::SIPMessage&, const std::string& raw_body);
    static int  get_content_length(const common::SIPMessage&);
    static std::string trim(const std::string& value);
};

}
```

### Router Interface

```cpp
namespace proxy {

    enum class RoutingAction {
        RespondLocally,
        ForwardRequest,
        ForwardResponse,
        Ignore
    };

    struct RoutingResult {
        RoutingAction           action  = RoutingAction::Ignore;
        common::SIPMessage      response;
        common::SIPMessage      message;
        std::optional<std::string> contact;
        std::string             ip;
        uint16_t                port = 0;
        std::string             user;
    };

    class SIPRouter {
    public:
        explicit SIPRouter(RegistrationTable& table);
        RoutingResult route(const common::SIPMessage& message,
                            const std::string& sender_ip,
                            uint16_t sender_port);
    };

}
```

### Rules

| Rule | Description |
|------|-------------|
| **Header order preserved** | `std::vector<SIPHeader>` guarantees insertion order. Code MUST NOT reorder headers. |
| **Case-sensitive names** | Header names are stored and compared exactly as received (e.g., `Via` ≠ `via`). |
| **Multiple same-name headers** | Allowed (e.g., multiple `Via` headers). Use `get_headers()` to retrieve all values. |
| **Content-Length** | MUST be updated via `update_content_length()` by the serializer before sending. |
| **No side-effects in `get_header`** | Accessors are `const` and MUST NOT modify state. |
| **Namespace** | All shared types live in `namespace common`. Do not pollute the global namespace. |

---

## Contract 3 — Proxy ↔ Client (SIP Message Requirements)

**Owner:** Group 2 (Proxy) + Group 3 (Client)  
**Client entry point:** `client/include/client/sip_client.hpp`  
**Message builder:** `client/include/client/sip_message_factory.hpp`

### SIPClient Public API (Proxy-relevant subset)

```cpp
class SIPClient {
public:
    SIPClient(const std::string& server_ip, int server_port);

    void start_transport();
    void send_to_server(const std::string& message);

    // Registration
    std::string build_register_message(const std::string& username,
                                       const std::string& domain,
                                       const std::string& extra_headers = {});
    void do_register(const std::string& username,
                     const std::string& domain,
                     const std::string& extra_headers = {});

    // Call control
    void do_invite(const std::string& from_user, const std::string& from_domain,
                   const std::string& to_user,   const std::string& to_domain);
    void do_answer();
    void do_reject();
    void do_bye(const std::string& extra_headers = {});

    // Header injection (for UI / testing)
    void set_pending_headers(const std::string& method, const std::string& headers);
    std::string pending_headers(const std::string& method) const;

    // Callbacks (set before run())
    using ResponseCallback = std::function<void(bool, const std::string&)>;
    void set_register_response_callback(ResponseCallback cb);

    using StateCallback = std::function<void(const std::string& state,
                                              const std::string& call_id,
                                              const std::string& remote_uri)>;
    void set_call_state_callback(StateCallback cb);
    void set_incoming_call_callback(
        std::function<void(const std::string& call_id,
                           const std::string& caller)> cb);
};
```

### SIPMessageFactory — Built Message Formats

```cpp
class SIPMessageFactory {
public:
    SIPMessageFactory(const std::string& local_ip, int local_port);

    std::string build_register(const std::string& username,
                               const std::string& domain,
                               const std::string& extra_headers = {});

    std::string build_invite(const std::string& from_user,
                             const std::string& from_domain,
                             const std::string& to_user,
                             const std::string& to_domain,
                             const std::string& call_id,
                             const std::string& extra_headers = {});

    std::string build_ack(/* same signature as invite */);
    std::string build_bye(/* same signature as invite */);

    std::string build_response(int code,
                               const std::string& reason,
                               const std::string& request_raw);

    std::string new_call_id() const;
};
```

### Required Headers per Method

#### REGISTER

| Header | Required | Notes |
|--------|----------|-------|
| `Via` | ✅ | `SIP/2.0/UDP <local_ip>:<local_port>;branch=z9hG4bK<random>` |
| `From` | ✅ | `<sip:username@domain>;tag=<random>` |
| `To` | ✅ | `<sip:username@domain>` |
| `Call-ID` | ✅ | Stable across re-registrations; use `register_call_id_` |
| `CSeq` | ✅ | Monotonically incrementing integer + ` REGISTER` |
| `Contact` | ✅ | `<sip:username@local_ip:local_port>` |
| `Content-Length` | ✅ | `0` for REGISTER (no body) |

#### INVITE

| Header | Required | Notes |
|--------|----------|-------|
| `Via` | ✅ | Fresh branch per transaction |
| `From` | ✅ | Caller identity |
| `To` | ✅ | Callee identity (no tag on initial INVITE) |
| `Call-ID` | ✅ | New unique ID per call; use `new_call_id()` |
| `CSeq` | ✅ | `1 INVITE` (reset per new Call-ID) |
| `Contact` | ✅ | Where to send subsequent in-dialog requests |
| `Content-Length` | ✅ | `0` if no SDP; length of SDP body otherwise |
| SDP body | ⬜ | Optional — may be empty |

### Call-ID Format

```
<random-hex>@<local_ip>
```

Generated by `SIPMessageFactory::new_call_id()` using a 64-bit Mersenne Twister (`std::mt19937_64`). Must be unique per call; REGISTER reuses the same Call-ID across re-registrations.

### Via Handling

- Client adds one `Via` header per outgoing request with a unique `branch` parameter.
- Proxy MUST prepend its own `Via` when forwarding requests downstream.
- Proxy MUST strip its own `Via` (top entry) before forwarding responses upstream.
- See `SIPRouter::build_proxy_via()` and `SIPRouter::strip_proxy_via()`.

### Contact Handling

- Client registers `sip:<username>@<local_ip>:<local_port>`.
- Proxy stores the Contact in `RegistrationTable` keyed by SIP identity (user part of `From`/`To`).
- On INVITE, the proxy resolves the callee Contact from `RegistrationTable` to determine the forwarding destination.

---

## Contract 4 — Header Injection Engine

**Owner:** Group 3 (Client) + Group 4 (UI)  
**UI widget:** `ui/include/ui/header_injection_widget.hpp`  
**Client integration:** `SIPClient::set_pending_headers()` / `SIPClient::pending_headers()`

### Logical Interface

The document specification defines the following `HeaderInjector` interface. The actual implementation is split across `HeaderInjectionWidget` (Qt UI) and `SIPClient` (client runtime):

```cpp
class HeaderInjector {
public:
    void add_header(const std::string& name, const std::string& value);
    void replace_header(const std::string& name, const std::string& value);
    void apply(common::SIPMessage& message);
};
```

### Qt Widget Interface (`HeaderInjectionWidget`)

```cpp
class HeaderInjectionWidget : public QWidget {
    Q_OBJECT
public:
    explicit HeaderInjectionWidget(QWidget* parent = nullptr);

    QString headersFor(const QString& method) const;  // returns raw header text for a SIP method
    bool    isEnabled() const;                         // whether injection is active

    void syncTo(HeaderInjectionWidget* other) const;   // mirror state to another widget

signals:
    void stateChanged();
};
```

The widget presents a tab per SIP method (from `kMethods` static list: `REGISTER`, `INVITE`, etc.) with a free-text `QTextEdit` per tab.

### Client Integration

```cpp
// Store headers to be injected for a specific method
client.set_pending_headers("INVITE", "X-Custom-Header: value\r\n");

// Retrieve stored headers before building the message
std::string extra = client.pending_headers("INVITE");
std::string msg   = client.factory().build_invite(..., extra);
```

### Rules

| Rule | Description |
|------|-------------|
| **`add_header`** | Appends the new header at the end of the header list. |
| **`replace_header`** | Replaces the **first** matching header (case-sensitive name match). If no match found, the header is appended as a new entry. |
| **Proxy logging — new header** | When the proxy observes a custom header not in the standard set, it MUST log `[CUSTOM HEADER]`. |
| **Proxy logging — replaced header** | When the proxy detects a header has been replaced, it MUST log `[HEADER REPLACED]`. |
| **Per-method scope** | Injected headers are scoped per SIP method; headers set for `INVITE` are not applied to `REGISTER`. |
| **UI ↔ Client sync** | `HeaderInjectionWidget::syncTo()` mirrors tab content to another widget instance for consistency between proxy and client views. |

---

## Contract 5 — Logging Format

**Owner:** All Groups  
**Header:** `common/include/common/logger.hpp`  
**Implementation:** `common/src/logger.cpp`

### Logger API

```cpp
namespace common {

    enum class LogLevel { DEBUG, INFO, WARN, ERROR };

    class Logger {
    public:
        static Logger& instance(const std::string& filename = "");

        // Full structured log
        void log(LogLevel level,
                 const std::string& component,
                 const std::string& call_id,
                 const std::string& state,
                 const std::string& message);

        // Log with SIPMessage (serializes message automatically)
        void log(LogLevel level,
                 const std::string& component,
                 const std::string& call_id,
                 const std::string& state,
                 const common::SIPMessage& sip_message);

        // Convenience overload (no level — defaults to INFO)
        void log(const std::string& component,
                 const std::string& call_id,
                 const std::string& state,
                 const std::string& message);

        void separator();   // writes a visual divider line
    };

}
```

### Log Line Format

```
[YYYY-MM-DD HH:MM:SS] [ThreadID] [Component] [CallID=<call_id>] [State] Message text
```

### Field Definitions

| Field | Type | Description |
|-------|------|-------------|
| `Timestamp` | `string` | Local time, format `YYYY-MM-DD HH:MM:SS` |
| `ThreadID` | `string` | OS thread identifier (e.g., `Thread-3`) |
| `Component` | `string` | Module name — one of: `Proxy`, `Router`, `Parser`, `Client`, `Transport`, `UI` |
| `CallID` | `string` | SIP Call-ID value, prefixed with `CallID=` |
| `State` | `string` | Current call/proxy state — see table below |
| `Message` | `string` | Free-form human-readable description |

### Valid State Values

| State | Used By | Meaning |
|-------|---------|---------|
| `INIT` | Proxy, Client | Before any transaction begins |
| `TRYING` | Proxy | `100 Trying` sent or received |
| `RINGING` | Proxy, Client | `180 Ringing` sent or received |
| `ESTABLISHED` | Proxy, Client | Call connected (`200 OK` + `ACK` exchanged) |
| `TERMINATED` | Proxy, Client | Call ended via `BYE` |
| `REGISTERED` | Client | Successful REGISTER response received |
| `CUSTOM HEADER` | Proxy | Non-standard header detected (see Contract 4) |
| `HEADER REPLACED` | Proxy | Existing header was replaced (see Contract 4) |

### Example Log Lines

```
[2026-02-12 14:32:10] [Thread-3] [Proxy] [CallID=abc123] [TRYING] Received INVITE
[2026-02-12 14:32:11] [Thread-3] [Proxy] [CallID=abc123] [RINGING] Forwarded 180 Ringing to caller
[2026-02-12 14:32:15] [Thread-1] [Client] [CallID=abc123] [ESTABLISHED] Call answered
[2026-02-12 14:32:20] [Thread-2] [Proxy] [CallID=abc123] [CUSTOM HEADER] X-App-Version: 1.0
[2026-02-12 14:32:20] [Thread-2] [Proxy] [CallID=abc123] [HEADER REPLACED] Contact header updated
```

### Rules

| Rule | Description |
|------|-------------|
| **Singleton logger** | Use `Logger::instance()` everywhere. Never construct a `Logger` directly. |
| **Thread safety** | `Logger` uses an internal `std::mutex`; safe to call from any thread concurrently. |
| **File output** | Logs are written to `logs/<filename>`. The directory is created automatically. |
| **No silent swallowing** | Every received packet, sent response, and state transition MUST produce a log entry. |
| **Separator** | Call `logger.separator()` at session boundaries (e.g., before processing a new call) for readability. |

---

## Appendix A — Call State Machine

**Header:** `proxy/include/proxy/call_state_machine.hpp`  
**States:** `proxy/include/proxy/call_state.hpp`

```
        INVITE received
INIT ─────────────────► TRYING
                            │
                    100 Trying sent
                            │
                            ▼
                        RINGING  ◄─── 180 Ringing
                            │
                    200 OK forwarded
                            │
                            ▼
                       ESTABLISHED ◄─── ACK received
                            │
                         BYE received
                            │
                            ▼
                       TERMINATED
```

### Events

| `CallEvent` | Trigger |
|-------------|---------|
| `INVITE` | INVITE request arrives at proxy |
| `TRYING_100` | Proxy sends 100 Trying |
| `RINGING_180` | 180 Ringing forwarded from callee |
| `OK_200` | 200 OK forwarded from callee |
| `ACK` | ACK received from caller |
| `BYE` | BYE received from either party |

---

## Appendix B — File Locations Quick Reference

| Contract | File(s) |
|----------|---------|
| Transport interface | `networking/include/networking/IUdpTransport.hpp` |
| Transport implementation | `networking/include/networking/udp_transport.hpp` + `.cpp` |
| Shared SIP types | `common/include/common/sip_message.hpp` + `.cpp` |
| SIP parser | `proxy/include/proxy/sip_parser.hpp` + `.cpp` |
| SIP router | `proxy/include/proxy/sip_router.hpp` + `.cpp` |
| Registration table | `proxy/include/proxy/registration_table.hpp` + `.cpp` |
| Call state machine | `proxy/include/proxy/call_state_machine.hpp` + `.cpp` |
| SIP client | `client/include/client/sip_client.hpp` + `.cpp` |
| Message factory | `client/include/client/sip_message_factory.hpp` + `.cpp` |
| Header injection widget | `ui/include/ui/header_injection_widget.hpp` + `.cpp` |
| Logger | `common/include/common/logger.hpp` + `.cpp` |
