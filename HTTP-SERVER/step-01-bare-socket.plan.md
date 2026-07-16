# Step 1: Bare Minimum Socket Connection

**Status:** Done

## Goal

Prove the accept/read loop works at all.

## Build

- Server binds to port 8080, listens, accepts one connection
- Print every byte received to console
- No HTTP response yet

## File

- `EchoServer.java`

## Compile and run

```powershell
cd "D:\BUILD OWN XYZ\HTTP-SERVER"
javac EchoServer.java
java EchoServer
```

## Test

**Terminal 1** — start server:
```powershell
java EchoServer
```

**Terminal 2** — send data:
```powershell
curl.exe --max-time 5 localhost:8080
```

Or with telnet:
```powershell
telnet localhost 8080
```

## Pass criteria

- Server binds to 8080 without error
- Client bytes appear in server console
- Server exits cleanly after client disconnects

## Next

Step 2 — send a hardcoded HTTP response.
