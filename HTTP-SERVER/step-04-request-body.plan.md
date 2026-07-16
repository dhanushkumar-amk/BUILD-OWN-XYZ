# Step 4: Handle the Request Body

**Status:** Done

## Goal

Correctly handle the trickiest part of parsing — knowing when to stop reading.

## Build

- After parsing headers, read `Content-Length` header (default 0 if missing)
- Read exactly that many bytes as the body
- Store body in request object as `String` or `byte[]`
- Print body to console for POST requests

## Key logic

```java
int contentLength = Integer.parseInt(headers.getOrDefault("Content-Length", "0"));
byte[] body = new byte[contentLength];
int totalRead = 0;
while (totalRead < contentLength) {
    int n = in.read(body, totalRead, contentLength - totalRead);
    if (n == -1) break;
    totalRead += n;
}
```

## Test

```powershell
curl.exe -X POST -d "hello=world" localhost:8080
```

**Expected console output:** body = `hello=world`

```powershell
curl.exe -X POST -d "name=Alice&age=30" localhost:8080
```

## Pass criteria

- POST body read completely and printed correctly
- GET requests (no body) still work
- No hang waiting for more bytes after body is read

## Next

Step 5 — basic routing with (method, path) lookup table.
