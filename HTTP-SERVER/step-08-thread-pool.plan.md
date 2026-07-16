# Step 8: Thread Pool

**Status:** Done

## Goal

Bounded, safe concurrency.

## Build

Replace `new Thread(...)` with `ExecutorService`:

```java
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

ExecutorService pool = Executors.newFixedThreadPool(4);

while (true) {
    Socket client = serverSocket.accept();
    pool.submit(() -> handleClient(client));
}
```

## Test

- Set pool size to 2
- Add `Thread.sleep(5000)` to all handlers
- Fire 5 concurrent curl requests

```powershell
1..5 | ForEach-Object { Start-Job { curl.exe localhost:8080/slow } }
```

## Pass criteria

- Only 2 requests run at once (pool size)
- Extra requests queue and complete once a worker frees up
- Server does not crash or spawn unbounded threads

## Next

Step 9 — HTTP keep-alive (persistent connections).
