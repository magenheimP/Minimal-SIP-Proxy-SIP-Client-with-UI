# SIP Grammar Reference (Simplified)

## 1. Overview

This document describes a simplified subset of the Session Initiation Protocol (SIP) used in this project.

The goal is not full RFC compliance, but a **minimal, consistent grammar** sufficient to support:

* REGISTER
* INVITE
* ACK
* BYE
* Basic SIP responses (100, 180, 200)

---

## 2. General Message Structure

A SIP message consists of:

Start-Line
Header1: value
Header2: value
...
HeaderN: value

(empty line)

Body (optional)

### Key Rules

* Each line ends with `\r\n`
* Headers and body are separated by an empty line (`\r\n`)
* Header format: `Name: value`
* Only the **first `:`** splits name and value
* Header order MUST be preserved
* Header names are **case-sensitive**
* Multiple headers with the same name are allowed

---

## 3. Start Line

### 3.1 Request Format

```
METHOD sip:user@domain SIP/2.0
```

Example:

```
INVITE sip:bob@localhost SIP/2.0
```

### 3.2 Response Format

```
SIP/2.0 Status-Code Reason-Phrase
```

Example:

```
SIP/2.0 200 OK
```

---

## 4. Supported Methods

The system supports the following SIP methods:

* REGISTER
* INVITE
* ACK
* BYE

---

## 5. Core Headers

### 5.1 Via

Tracks the transport path.

Example:

```
Via: SIP/2.0/UDP 192.168.1.10:5060
```

---

### 5.2 From

Identifies the caller.

Example:

```
From: <sip:alice@localhost>
```

---

### 5.3 To

Identifies the callee.

Example:

```
To: <sip:bob@localhost>
```

---

### 5.4 Call-ID

Unique identifier for a SIP session.

Example:

```
Call-ID: abc123@localhost
```

---

### 5.5 CSeq

Sequence number and method.

Example:

```
CSeq: 1 INVITE
```

---

### 5.6 Contact

Specifies where the user can be reached.

Example:

```
Contact: <sip:alice@192.168.1.10>
```

---

### 5.7 Content-Length

Length of the message body in bytes.

Example:

```
Content-Length: 0
```

### Rules

* MUST always be present
* MUST be updated by serializer
* MUST match actual body size

---

## 6. Minimal Message Examples

### 6.1 REGISTER

```
REGISTER sip:localhost SIP/2.0
Via: SIP/2.0/UDP 192.168.1.10:5060
From: <sip:alice@localhost>
To: <sip:alice@localhost>
Call-ID: reg123
CSeq: 1 REGISTER
Contact: <sip:alice@192.168.1.10>
Content-Length: 0
```

---

### 6.2 INVITE

```
INVITE sip:bob@localhost SIP/2.0
Via: SIP/2.0/UDP 192.168.1.10:5060
From: <sip:alice@localhost>
To: <sip:bob@localhost>
Call-ID: call123
CSeq: 1 INVITE
Contact: <sip:alice@192.168.1.10>
Content-Length: 0
```

---

## 7. Basic Call Flow

### Call Setup

1. Client A → Proxy: INVITE

2. Proxy → Client B: INVITE

3. Client B → Proxy: 100 Trying

4. Proxy → Client A: 100 Trying

5. Client B → Proxy: 180 Ringing

6. Proxy → Client A: 180 Ringing

7. Client B → Proxy: 200 OK

8. Proxy → Client A: 200 OK

9. Client A → Proxy: ACK

10. Proxy → Client B: ACK

Call is now established.

---

### Call Termination

1. Client A → Proxy: BYE

2. Proxy → Client B: BYE

3. Client B → Proxy: 200 OK

4. Proxy → Client A: 200 OK

Call is terminated.

---

## 8. Header Injection (Client Feature)

The client can modify outgoing messages by:

### Adding Headers

```
X-Debug-ID: 777
```

Rules:

* Appended to the end of header list
* Must be preserved by proxy

---

### Replacing Headers

Example:

```
Via: SIP/2.0/UDP 10.0.0.99:5060
```

Rules:

* Replace first matching header
* If not found → add new header

---

## 9. Proxy Requirements for Headers

The proxy MUST:

* Parse all headers correctly
* Preserve header order
* Forward all headers unchanged (unless explicitly modified)
* Support duplicate headers
* Log:

```
[CUSTOM HEADER] X-Debug-ID: 777
[HEADER MODIFIED] Via: ...
```

---

## 10. Parsing Rules

The SIP parser MUST follow these steps:

1. Split message by `\r\n`
2. First line → Start-Line
3. Read headers until empty line
4. For each header:

    * Split at first `:`
    * Trim whitespace
5. After empty line:

    * Remaining bytes = body
6. Validate Content-Length

---

## 11. Serialization Rules

When rebuilding a SIP message:

* Preserve header order
* Preserve duplicate headers
* Recalculate `Content-Length`
* Ensure correct formatting:

```
Start-Line\r\n
Header: value\r\n
...\r\n
\r\n
Body
```

---

## 12. Non-Goals (Out of Scope)

This project does NOT implement:

* Full SIP RFC compliance
* Authentication (Digest, etc.)
* TCP transport (optional extension)
* SDP parsing (body may remain empty)
* Advanced routing logic

---

## 13. Summary

This simplified SIP grammar ensures:

* Predictable parsing
* Correct message forwarding
* Support for header injection
* Compatibility between client and proxy

It is intentionally minimal to focus on:

* Networking
* Concurrency
* System design
