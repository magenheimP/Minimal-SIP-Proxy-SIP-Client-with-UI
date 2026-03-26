# SIP Grammar Documentation

## Overview

This document provides comprehensive documentation of the SIP (Session Initiation Protocol) message grammar and parsing implementation used in the Minimal SIP Proxy & SIP Client project. The SIP Grammar defines the structure and format of all SIP messages, including requests, responses, headers, and body content.

## Table of Contents

1. [SIP Message Structure](#sip-message-structure)
2. [Start Line Grammar](#start-line-grammar)
3. [Header Grammar](#header-grammar)
4. [Message Body Grammar](#message-body-grammar)
5. [SIP Parser Implementation](#sip-parser-implementation)
6. [Parsing Rules](#parsing-rules)
7. [Message Components](#message-components)
8. [Common Header Fields](#common-header-fields)
9. [Examples](#examples)
10. [Parser Methods Reference](#parser-methods-reference)

---

## SIP Message Structure

The fundamental SIP message grammar consists of three primary components:

```
SIP-Message = Start-Line
              *( Header CRLF )
              CRLF
              [ Message-Body ]
```

### Component Breakdown

- **Start-Line**: Either a request line (for requests) or a status line (for responses)
- **Headers**: Zero or more SIP header fields, each terminated by CRLF
- **Blank Line**: CRLF that separates headers from body
- **Message-Body**: Optional message content (typically SDP for media negotiation)

### Message Delimiters

The parser recognizes two types of message delimiters:

| Delimiter | Type | Usage |
|-----------|------|-------|
| `\r\n\r\n` | Strict CRLF | RFC 3261 compliant, preferred |
| `\n\n` | LF only | Lenient parsing, accepted for compatibility |

---

## Start Line Grammar

### Request Line Format

```
Request-Line = Method SP Request-URI SP SIP-Version CRLF

Example:
INVITE sip:bob@biloxi.com SIP/2.0
```

**Components:**

- **Method**: SIP method name (INVITE, REGISTER, BYE, etc.)
- **Request-URI**: Target SIP URI
- **SIP-Version**: Protocol version (typically "SIP/2.0")

### Response Line Format

```
Status-Line = SIP-Version SP Status-Code SP Reason-Phrase CRLF

Example:
SIP/2.0 200 OK
```

**Components:**

- **SIP-Version**: Protocol version
- **Status-Code**: Three-digit numeric code (1xx, 2xx, 3xx, 4xx, 5xx, 6xx)
- **Reason-Phrase**: Human-readable explanation of status

### Start-Line Parsing

The parser stores the entire start line as-is without decomposition:

```cpp
message.start_line = line;  // Complete start line stored as string
```

This approach preserves the exact format and allows downstream components to parse request or response details as needed.

---

## Header Grammar

### Header Field Format

```
Header-Field = Field-Name ":" Field-Value CRLF

Example:
Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bK776asdhds
From: sip:alice@atlanta.com
To: sip:bob@biloxi.com
```

### Header Structure

| Component | Description |
|-----------|-------------|
| **Field-Name** | Case-insensitive header name, preceding the colon |
| **Colon** | Separates field name from value (`:`) |
| **Field-Value** | Header value, may contain whitespace and special characters |
| **CRLF** | Line terminator |

### Whitespace Handling

The parser trims leading and trailing whitespace from both field names and values:

```cpp
const std::string name = trim(line.substr(0, separator_pos));
const std::string value = trim(line.substr(separator_pos + 1));
```

**Trimming Rules:**

- **Trim characters**: Space (` `) and tab (`\t`)
- **Location**: Both leading and trailing positions
- **Example**: `"  From:  sip:alice@atlanta.com  "` becomes name=`"From"`, value=`"sip:alice@atlanta.com"`

### Multiple Headers with Same Name

SIP allows multiple header fields with the same name:

```
Route: <sip:proxy1.atlanta.com>
Route: <sip:proxy2.biloxi.com>
Route: <sip:proxy3.chicago.com>
```

These are stored as separate entries in the headers vector and can be retrieved as a collection.

---

## Message Body Grammar

### Body Structure

```
Message-Body = *OCTET
```

The message body is optional and contains arbitrary octets. Common body formats include:

- **SDP (Session Description Protocol)**: For media description
- **Plain text**: For messaging
- **XML**: For presence and event notifications
- **ISUP**: For telephony signaling

### Body Length Determination

The parser respects the **Content-Length** header to determine body size:

```cpp
int content_length = get_content_length(message);

if (content_length < 0) {
    // No Content-Length header; use entire remaining body
    message.body = raw_body;
} else {
    // Use only content_length bytes
    std::size_t safe_length = std::min(raw_body.size(), 
                                       static_cast<std::size_t>(content_length));
    message.body = raw_body.substr(0, safe_length);
}
```

### Content-Length Header

- **Presence**: Optional but recommended
- **Value**: Integer representing byte count of message body
- **Handling**: If absent, entire remaining data is treated as body
- **Safety**: Parser clamps body length to prevent buffer overruns

---

## SIP Parser Implementation

### Parser Class: `SIPParser`

Located in: `proxy/include/proxy/sip_parser.hpp` and `proxy/src/sip_parser.cpp`

### Parser Architecture

The `SIPParser` class uses a **top-down, multi-stage parsing approach**:

```cpp
class SIPParser {
public:
    static common::SIPMessage parse(const std::string& raw_message);
    
private:
    static void parse_start_line(common::SIPMessage& message,
                                const std::string& line);
    
    static void parse_header_line(common::SIPMessage& message,
                                   const std::string& line);
    
    static void parse_body(common::SIPMessage& message,
                          const std::string& raw_body);
    
    static int get_content_length(const common::SIPMessage& message);
    
    static std::string trim(const std::string& value);
};
```

---

## Parsing Rules

### Rule 1: Message Separation (Headers from Body)

The parser identifies the separator between headers and message body:

```cpp
std::size_t separator_pos = raw_message.find("\r\n\r\n");
std::size_t separator_length = 4;

if (separator_pos == std::string::npos) {
    separator_pos = raw_message.find("\n\n");
    separator_length = 2;
}
```

**Logic:**
1. First, attempt to find strict CRLF separator (`\r\n\r\n`)
2. If not found, fall back to lenient LF separator (`\n\n`)
3. If neither found, treat entire message as headers (no body)

### Rule 2: Start Line Processing

The first non-empty line of the message is treated as the start line:

```cpp
bool first_line = true;

while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();  // Remove trailing CR if present
    }
    
    if (line.empty()) {
        continue;
    }
    
    if (first_line) {
        parse_start_line(message, line);
        first_line = false;
        continue;
    }
    
    parse_header_line(message, line);
}
```

**Processing:**
- Skip empty lines at the beginning
- Designate first non-empty line as start line
- Remove any trailing carriage return
- Process remaining lines as headers

### Rule 3: Header Line Parsing

Each header line is parsed by splitting on the first colon:

```cpp
const std::size_t separator_pos = line.find(':');

if (separator_pos == std::string::npos) {
    return;  // Invalid header; skip it
}

const std::string name = trim(line.substr(0, separator_pos));
const std::string value = trim(line.substr(separator_pos + 1));

if (name.empty()) {
    return;  // Empty name; skip
}

message.add_header(name, value);
```

**Processing:**
1. Find the first colon in the line
2. Text before colon = header name
3. Text after colon = header value
4. Trim whitespace from both
5. Reject if name is empty
6. Add to message headers

### Rule 4: Body Handling

The body is extracted and length-validated:

```cpp
const int content_length = get_content_length(message);

if (content_length < 0) {
    message.body = raw_body;
} else {
    const std::size_t safe_length = std::min(raw_body.size(), 
                                             static_cast<std::size_t>(content_length));
    message.body = raw_body.substr(0, safe_length);
}
```

**Processing:**
1. Query Content-Length header
2. If absent or invalid, use entire raw_body
3. If present, extract only the specified number of bytes
4. Safely handle cases where raw_body is shorter than Content-Length

### Rule 5: Carriage Return Handling

The parser is lenient with line endings:

```cpp
if (!line.empty() && line.back() == '\r') {
    line.pop_back();
}
```

**Behavior:**
- Lines ending with `\r\n` have the `\r` stripped by `std::getline`
- Lines ending with `\n` are handled normally
- The parser removes any remaining trailing `\r`

---

## Message Components

### SIPMessage Structure

```cpp
struct SIPMessage {
    std::string start_line;
    std::vector<SIPHeader> headers;
    std::string body;
    
    // Header operations
    std::string get_header(const std::string& name) const;
    void set_header(const std::string& name, const std::string& value);
    void add_header(const std::string& name, const std::string& value);
    bool has_header(const std::string& name) const;
    std::vector<std::string> get_headers(const std::string& name) const;
    void prepend_header(const std::string& name, const std::string& value);
    bool remove_first_header(const std::string& name);
    
    // Message type detection
    bool is_request() const;
    bool is_response() const;
    
    // Content extraction
    std::string get_method() const;
    int get_status_code() const;
    
    // Serialization
    void update_content_length();
    std::string serialize() const;
};
```

### SIPHeader Structure

```cpp
struct SIPHeader {
    std::string name;
    std::string value;
};
```

---

## Common Header Fields

### Mandatory Headers

| Header | Purpose | Format |
|--------|---------|--------|
| **Via** | Route tracking, response routing | `Via: SIP/2.0/UDP host[:port]` |
| **From** | Message originator | `From: <sip:user@domain>;tag=xyz` |
| **To** | Message recipient | `To: <sip:user@domain>;tag=abc` |
| **Call-ID** | Unique call identifier | `Call-ID: abc123@host.com` |
| **CSeq** | Sequence number and method | `CSeq: 1 INVITE` |

### Important Headers

| Header | Purpose | Format |
|--------|---------|--------|
| **Contact** | Return address for requests | `Contact: <sip:user@host>` |
| **Content-Type** | Body MIME type | `Content-Type: application/sdp` |
| **Content-Length** | Body byte count | `Content-Length: 1234` |
| **Route** | Routing information | `Route: <sip:proxy@host>` |
| **Expires** | Expiration time | `Expires: 3600` |

### Request Methods

The most common SIP request methods include:

| Method | Purpose | Direction |
|--------|---------|-----------|
| **INVITE** | Session initiation | Client → Server |
| **REGISTER** | User registration | Client → Server |
| **BYE** | Session termination | Either direction |
| **CANCEL** | Request cancellation | Client → Server |
| **OPTIONS** | Server capabilities | Either direction |
| **ACK** | INVITE confirmation | Client → Server |

### Response Status Codes

SIP response status codes follow standard conventions:

| Range | Category | Examples |
|-------|----------|----------|
| **1xx** | Provisional | 100 Trying, 180 Ringing |
| **2xx** | Success | 200 OK, 202 Accepted |
| **3xx** | Redirection | 300 Multiple Choices, 301 Moved |
| **4xx** | Client Error | 400 Bad Request, 401 Unauthorized |
| **5xx** | Server Error | 500 Server Internal Error |
| **6xx** | Global Failure | 600 Busy Everywhere |

---

## Examples

### Example 1: INVITE Request

```
INVITE sip:bob@biloxi.com SIP/2.0
Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bK776asdhds
Max-Forwards: 70
To: Bob <sip:bob@biloxi.com>
From: Alice <sip:alice@atlanta.com>;tag=1928301774
Call-ID: a84b4c76e66710@pc33.atlanta.com
CSeq: 314159 INVITE
Contact: <sip:alice@pc33.atlanta.com>
Content-Type: application/sdp
Content-Length: 142

v=0
o=UserA 2890844526 2890844527 IN IP4 192.0.2.1
s=Session SDP
c=IN IP4 192.0.2.1
t=0 0
m=audio 49172 RTP/AVP 0
a=rtpmap:0 PCMU/8000
```

**Parsing Breakdown:**
- **Start Line**: `INVITE sip:bob@biloxi.com SIP/2.0`
- **Headers**: 8 header fields (Via, Max-Forwards, To, From, Call-ID, CSeq, Contact, Content-Type, Content-Length)
- **Body**: 6 lines of SDP (Session Description Protocol)

### Example 2: Response Message

```
SIP/2.0 200 OK
Via: SIP/2.0/UDP server10.biloxi.com;branch=z9hG4bKnashds8
Via: SIP/2.0/UDP bigbox3.site3.atlanta.com
Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bK776asdhds
To: Bob <sip:bob@biloxi.com>;tag=a6c85cf
From: Alice <sip:alice@atlanta.com>;tag=1928301774
Call-ID: a84b4c76e66710@pc33.atlanta.com
CSeq: 314159 INVITE
Contact: <sip:bob@192.0.2.4>
Content-Type: application/sdp
Content-Length: 131

v=0
o=UserB 2890844526 2890844527 IN IP4 192.0.2.4
s=Session SDP
c=IN IP4 192.0.2.4
t=0 0
m=audio 49172 RTP/AVP 0
a=rtpmap:0 PCMU/8000
```

**Parsing Breakdown:**
- **Start Line**: `SIP/2.0 200 OK` (Response)
- **Via Headers**: 3 entries (stacked from route)
- **Body**: SDP from recipient

### Example 3: REGISTER Request

```
REGISTER sip:registrar.atlanta.com SIP/2.0
Via: SIP/2.0/UDP client.atlanta.com:5060;branch=z9hG4bKnashds7
Max-Forwards: 70
From: <sip:user@atlanta.com>;tag=xyz
To: <sip:user@atlanta.com>
Call-ID: xyz@client.atlanta.com
CSeq: 1 REGISTER
Contact: <sip:user@client.atlanta.com>
Expires: 3600
Content-Length: 0

```

**Parsing Breakdown:**
- **Start Line**: `REGISTER sip:registrar.atlanta.com SIP/2.0`
- **Headers**: Standard registration headers
- **Body**: None (Content-Length: 0)

---

## Parser Methods Reference

### Public Methods

#### `parse(const std::string& raw_message)`

```cpp
static common::SIPMessage parse(const std::string& raw_message);
```

**Purpose**: Parse a complete SIP message from raw string

**Parameters:**
- `raw_message`: Raw SIP message string (may contain `\r\n` or `\n` line endings)

**Returns**: `common::SIPMessage` object with populated fields

**Algorithm:**
1. Find header/body separator
2. Extract header and body sections
3. Parse start line
4. Parse header lines
5. Parse message body
6. Return structured message

**Example:**
```cpp
std::string raw = "INVITE sip:bob@biloxi.com SIP/2.0\r\nVia: SIP/2.0/UDP pc33\r\n\r\n";
common::SIPMessage msg = SIPParser::parse(raw);
```

---

### Private Methods

#### `parse_start_line(common::SIPMessage& message, const std::string& line)`

```cpp
static void parse_start_line(common::SIPMessage& message,
                            const std::string& line);
```

**Purpose**: Parse the first line of a SIP message

**Parameters:**
- `message`: Reference to SIPMessage to populate
- `line`: The start line string

**Behavior:**
- Stores the entire line as-is in `message.start_line`
- No decomposition or validation
- Allows downstream processing to determine request vs. response

**Note**: The actual decomposition (method/URI/version or version/code/phrase) is done by higher-level handlers, not this parser.

---

#### `parse_header_line(common::SIPMessage& message, const std::string& line)`

```cpp
static void parse_header_line(common::SIPMessage& message,
                              const std::string& line);
```

**Purpose**: Parse a single header line

**Parameters:**
- `message`: Reference to SIPMessage to populate
- `line`: A single header line (e.g., "From: sip:alice@atlanta.com")

**Algorithm:**
1. Find the colon (`:`)
2. If not found, skip the line
3. Extract name and value using colon position
4. Trim whitespace from both
5. If name is empty, skip
6. Otherwise, add header to message

**Behavior:**
- Supports multiple headers with the same name
- Case-preserves field names
- Preserves field value exactly (except whitespace trimming)

---

#### `parse_body(common::SIPMessage& message, const std::string& raw_body)`

```cpp
static void parse_body(common::SIPMessage& message,
                      const std::string& raw_body);
```

**Purpose**: Extract and validate message body

**Parameters:**
- `message`: Reference to SIPMessage to populate (may contain headers)
- `raw_body`: Raw body string extracted from message

**Algorithm:**
1. If raw_body is empty, set message.body to empty and return
2. Query Content-Length header
3. If Content-Length is absent or invalid, use entire raw_body
4. If Content-Length is present:
   - Clamp length to actual raw_body size
   - Extract substring of specified length
5. Set message.body

**Safety:**
- Prevents buffer overruns by clamping to actual size
- Handles missing Content-Length gracefully
- Rejects malformed Content-Length values

---

#### `get_content_length(const common::SIPMessage& message)`

```cpp
static int get_content_length(const common::SIPMessage& message);
```

**Purpose**: Extract Content-Length from message headers

**Parameters:**
- `message`: SIPMessage to query

**Returns:**
- Integer body length if Content-Length header is present and valid
- `-1` if absent or unparseable

**Algorithm:**
1. Query "Content-Length" header (case-sensitive)
2. Return -1 if header is empty or not found
3. Attempt to parse as integer using `std::stoi`
4. Return parsed value or -1 on exception

**Note**: Header lookup is case-sensitive in the current implementation.

---

#### `trim(const std::string& value)`

```cpp
static std::string trim(const std::string& value);
```

**Purpose**: Remove leading and trailing whitespace

**Parameters:**
- `value`: String to trim

**Returns**: Trimmed string (or empty string if all whitespace)

**Algorithm:**
1. Find first non-whitespace character (not space, not tab)
2. Find last non-whitespace character
3. Return substring between these positions
4. Return empty string if no non-whitespace found

**Whitespace Characters Trimmed:**
- Space (` `, ASCII 32)
- Tab (`\t`, ASCII 9)

**Example:**
- Input: `"  From: alice  "`
- Output: `"From: alice"`

---

## Error Handling

### Parser Robustness

The SIP parser is designed to be lenient and forgiving:

| Condition | Behavior |
|-----------|----------|
| Invalid header (no colon) | Header line is skipped silently |
| Empty header name | Header line is skipped silently |
| Missing Content-Length | Entire remaining data used as body |
| Invalid Content-Length | Entire remaining data used as body |
| No separator found | Entire message treated as headers |
| Mixed line endings | Both `\r\n` and `\n` accepted |
| Trailing `\r` in lines | Automatically removed |
| Empty lines between headers | Skipped silently |

### Exception Handling

The parser uses try-catch for Content-Length parsing:

```cpp
try {
    return std::stoi(value);  // Convert to integer
} catch (...) {
    return -1;  // Return -1 on any exception
}
```

This prevents crashes from malformed numeric values.

---

## Integration Points

### Consumer Components

#### SIP Proxy (`proxy/src/`)
- Uses `SIPParser::parse()` to deserialize received UDP packets
- Processes parsed messages for routing and forwarding
- Modifies headers and re-serializes for transmission

#### SIP Client (`client/src/`)
- Uses `SIPParser::parse()` to handle received responses
- Processes parsed messages for session management
- Generates requests using `SIPMessageFactory`

#### Tests (`tests/parser_tests.cpp`)
- Unit tests for parser functionality
- Validates parsing of various SIP messages
- Tests error conditions and edge cases

---

## RFC Compliance

This implementation is based on **RFC 3261 (SIP: Session Initiation Protocol)** with the following notes:

### Compliant Areas
- Message structure (start line, headers, body)
- Header format and parsing
- Content-Length handling
- Support for multiple headers with same name

### Lenient Areas
- Accepts both `\r\n` and `\n` line endings (RFC requires `\r\n`)
- Stores start line as-is without full parsing (allows flexibility)
- No validation of header field values against grammar

### Not Implemented
- Strict grammar validation for all header fields
- Compact header form (e.g., `f` for `From`)
- Header field parameter parsing
- SIP URI parsing
- Authentication challenge handling

---

## Performance Considerations

### Time Complexity
- **Message parsing**: O(n) where n = total message length
- **Header lookup**: O(h) where h = number of headers
- **Start-line parsing**: O(1) (no parsing, just storage)

### Space Complexity
- **Message storage**: O(n) (entire message in memory)
- **Header vector**: O(h) (linear with header count)

### Optimization Opportunities
1. **Lazy parsing**: Don't parse body until accessed
2. **Header indexing**: Create map for O(1) header lookup
3. **Streaming**: Process message in chunks rather than loading all at once
4. **Compact form**: Support compressed header notation

---

## Future Enhancements

1. **Full Start-Line Decomposition**
   - Parse request line into method, URI, version
   - Parse status line into code and reason
   - Validate against SIP grammar

2. **Header Field Validation**
   - Validate format of specific headers (Via, From, To, etc.)
   - Parse header parameters (tags, branches, etc.)
   - Enforce mandatory headers

3. **URI Parsing**
   - Full SIP URI parsing and validation
   - Support for tel: and mailto: schemes

4. **Content-Negotiation**
   - Parse Content-Type header
   - Validate Content-Type vs. body content

5. **Security**
   - Add hooks for message authentication
   - Support SIP identity verification

---

## References

- **RFC 3261**: SIP: Session Initiation Protocol
- **RFC 3262**: Reliability of Provisional Responses in SIP
- **RFC 3265**: Session Initiation Protocol (SIP) Event Notification
- **SDP (Session Description Protocol)**: RFC 4566

---

## Document Metadata

| Attribute | Value |
|-----------|-------|
| **Version** | 1.0 |
| **Last Updated** | March 2026 |
| **Author** | Documentation Team |
| **Project** | Minimal SIP Proxy & SIP Client |
| **Status** | Complete |

---

**End of SIP Grammar Documentation**
