# Step 2: Send a Hardcoded HTTP Response

**Status:** Done

## Goal

Learn the exact HTTP response format by making a real client (browser) accept it.

## Build

After accepting a connection:

1. Read and discard incoming bytes (keep the read loop from Step 1, or drain until client stops sending)
2. Write back a valid hardcoded HTTP response via `socket.getOutputStream()`
3. Close the connection

### Response format

```
HTTP/1.1 200 OK\r\n
Content-Length: 13\r\n
Content-Type: text/plain\r\n
\r\n
Hello, World!
```

Rules:
- Lines end with `\r\n` (CRLF), not just `\n`
- Blank line (`\r\n`) separates headers from body
- `Content-Length` must match exact byte count of the body (`"Hello, World!"` = 13 bytes)

## Changes to EchoServer.java

```java
import java.io.InputStream;
import java.io.OutputStream;
// ... existing imports

// After accept, drain input (optional but good practice):
try (InputStream in = client.getInputStream();
     OutputStream out = client.getOutputStream()) {

    byte[] buffer = new byte[1024];
    while (in.read(buffer) != -1) { /* discard */ }

    String body = "Hello, World!";
    String response =
        "HTTP/1.1 200 OK\r\n" +
        "Content-Length: " + body.length() + "\r\n" +
        "Content-Type: text/plain\r\n" +
        "\r\n" +
        body;

    out.write(response.getBytes());
    out.flush();
}
```

## Compile and run

```powershell
javac EchoServer.java
java EchoServer
```

## Test

### Test A — Browser

Open `http://localhost:8080` in Chrome/Firefox/Edge.

**Expected:** Page shows `Hello, World!`

### Test B — curl

```powershell
curl.exe localhost:8080
```

**Expected output:**
```
Hello, World!
```

### Test C — curl verbose (check headers)

```powershell
curl.exe -v localhost:8080
```

**Expected:** `HTTP/1.1 200 OK`, `Content-Length: 13`, body `Hello, World!`

## Pass criteria

- Browser renders the hardcoded body (not a connection error or blank page)
- `curl` receives `200 OK` and correct body
- Server still handles only one connection then exits (same as Step 1)

## Common mistakes

- Using `\n` instead of `\r\n` — browsers may not parse headers correctly
- Wrong `Content-Length` — body gets truncated or client hangs
- Forgetting `out.flush()` — response may not send immediately

## Next

Step 3 — parse the incoming request line and headers.
