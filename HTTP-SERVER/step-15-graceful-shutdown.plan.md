# Step 15: Graceful Shutdown (Optional)

**Status:** Done

## Goal

Production-shaped behavior — not just "it works until you kill -9 it."

## Build

1. Register a shutdown hook for Ctrl+C / SIGTERM:

```java
Runtime.getRuntime().addShutdownHook(new Thread(() -> {
    System.out.println("Shutting down...");
    serverSocket.close();       // stop accepting
    pool.shutdown();            // no new tasks
    pool.awaitTermination(30, TimeUnit.SECONDS); // let in-flight finish
}));
```

2. Stop accepting new connections
3. Let in-flight requests complete
4. Exit cleanly

## Test

1. Add a `/slow` handler with 5-second delay
2. Start a slow request: `curl.exe localhost:8080/slow`
3. Press **Ctrl+C** while it's in flight

## Pass criteria

- Slow request finishes before process exits
- No new connections accepted after shutdown signal
- Clean exit (no stack trace, no hung threads)

## Milestone

Full HTTP server build complete. All 15 steps done.
