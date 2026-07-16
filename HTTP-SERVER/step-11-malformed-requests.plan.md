# Step 11: Malformed Requests

**Status:** Done

## Goal

Robustness against bad input, not just bad clients.

## Build

- Wrap request parsing in try/catch
- On parse failure, respond with `400 Bad Request` and close connection
- Server thread must not crash; accept loop keeps running

```java
try {
    HttpRequest request = parseRequest(in);
    // handle normally
} catch (Exception e) {
    sendError(out, 400, "Bad Request");
}
```

## Test

```powershell
telnet localhost 8080
```

Type random garbage:
```
asdfjkl;qwerty total nonsense !!!
```

## Pass criteria

- Server responds `HTTP/1.1 400 Bad Request`
- Connection closes cleanly
- Server still accepts new valid requests afterward

## Milestone

After Step 11 the server is **robust enough to not embarrass itself under bad input**.

## Next

Step 12 — path parameters and query strings.
