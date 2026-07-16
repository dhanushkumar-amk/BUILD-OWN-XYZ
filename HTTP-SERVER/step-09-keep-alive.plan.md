# Step 9: Keep-Alive (Persistent Connections)

**Status:** Done

## Goal

Match real-world HTTP/1.1 behavior.

## Build

- After handling one request, loop back and read the **next** request on the same socket
- Keep connection open unless:
  - Client sends `Connection: close`
  - HTTP/1.0 without keep-alive
- Add `Connection: keep-alive` header in responses

```java
while (!shouldClose) {
    HttpRequest request = parseRequest(in);
    String response = route(request);
    out.write(response.getBytes());
    out.flush();
    shouldClose = request.headers.get("Connection").equals("close");
}
```

## Test

```powershell
curl.exe -v localhost:8080/
curl.exe -v localhost:8080/about
```

Check server logs — same connection should serve multiple requests.

## Pass criteria

- One TCP connection serves multiple HTTP requests
- Log shows same client address for sequential requests from `curl -v`
- `Connection: close` properly closes the socket

## Milestone

After Step 9 you have a **genuinely functional, concurrent, HTTP/1.1-compliant server**.

## Next

Step 10 — socket read timeouts.
