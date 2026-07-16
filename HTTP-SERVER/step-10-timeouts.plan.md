# Step 10: Timeouts

**Status:** Done

## Goal

Defend against slow/dead/malicious clients.

## Build

Set socket read timeout on each accepted connection:

```java
client.setSoTimeout(5000); // 5 seconds
```

Catch `SocketTimeoutException` and close the idle connection.

## Test

```powershell
telnet localhost 8080
```

Connect but **don't type anything**. Wait 5+ seconds.

## Pass criteria

- Server closes idle connection after timeout
- Thread is freed (not blocked forever)
- Server keeps accepting new connections after timeout

## Next

Step 11 — handle malformed requests gracefully.
