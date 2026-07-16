# Step 14: Logging

**Status:** Done

## Goal

Operational visibility + hands-on concurrency-safety practice.

## Build

Log each request after handling:

```
GET /users/42 200 12ms
POST /data 201 45ms
```

Fields: method, path, status code, response time.

Use thread-safe logging:

```java
// Simple approach — synchronized block
synchronized (logLock) {
    System.out.println(logLine);
}
```

Or `java.util.logging.Logger`.

## Test

Fire concurrent requests:

```powershell
1..10 | ForEach-Object { Start-Job { curl.exe -s localhost:8080/ } }
```

## Pass criteria

- Every request produces one log line
- Log lines are not garbled/interleaved mid-line
- Status codes and timing are accurate

## Next

Step 15 — graceful shutdown.
