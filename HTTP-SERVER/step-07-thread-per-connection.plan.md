# Step 7: Thread Per Connection

**Status:** Done

## Goal

Feel the concurrency problem get solved, not just read about it.

## Build

- In the accept loop, spawn a new `Thread` for each connection
- Move `handleClient()` into the thread's `run()` method
- Add an artificial delay in one handler (e.g. `Thread.sleep(3000)`)

```java
while (true) {
    Socket client = serverSocket.accept();
    new Thread(() -> handleClient(client)).start();
}
```

## Test

Add `Thread.sleep(3000)` to the `/slow` handler, then fire parallel requests:

```powershell
curl.exe localhost:8080/slow &
curl.exe localhost:8080/
curl.exe localhost:8080/
```

Or run 3 curl commands in separate terminals at the same time.

## Pass criteria

- `/` requests complete immediately even while `/slow` is sleeping
- Requests are served concurrently, not queued behind the slow one

## Next

Step 8 — replace raw threads with a fixed-size thread pool.
