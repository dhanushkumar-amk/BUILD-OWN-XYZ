# Step 6: Multiple Sequential Clients (Single-Threaded)

**Status:** Done

## Goal

A server that survives more than one request, even if still one-at-a-time.

## Build

Wrap `accept()` in a `while (true)` loop:

```java
while (true) {
    Socket client = serverSocket.accept();
    handleClient(client);
}
```

Extract connection handling into a `handleClient(Socket)` method.

## Test

```powershell
# With server running, run these one after another:
curl.exe localhost:8080/
curl.exe localhost:8080/about
curl.exe localhost:8080/missing
```

Server should **not** exit after the first request.

## Pass criteria

- Server keeps running after first client disconnects
- Each sequential request gets the correct response
- No need to restart server between requests

## Next

Step 7 — thread per connection for concurrency.
