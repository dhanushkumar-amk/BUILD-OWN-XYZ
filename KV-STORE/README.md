# Mini Key-Value Store

A durable, in-memory key-value database written in Go. Data lives in memory for fast reads and writes, and every change is persisted to a write-ahead log (WAL) on disk so it survives restarts and crashes.

The project was built step-by-step from a plain Go map to a networked database with log replay, fsync durability, crash recovery, and log compaction.

---

## Features

- **In-memory store** — fast `SET`, `GET`, `DELETE`, and `KEYS` operations
- **Write-ahead logging** — every `SET` and `DELETE` is appended to a log file
- **Crash recovery** — on startup, the log is replayed to rebuild in-memory state
- **Durability** — each write is flushed to disk with `fsync` before success is returned
- **Corruption handling** — a malformed or incomplete last log line is skipped during replay
- **Log compaction** — old redundant entries are collapsed into a single snapshot per key
- **Two interfaces** — interactive CLI and HTTP JSON API
- **Thread-safe** — concurrent reads/writes protected with `sync.RWMutex`

---

## Requirements

- **Go 1.22+**
- Windows, macOS, or Linux

---

## Quick Start

```powershell
cd KV-STORE

# Run tests
go test ./...

# Build
go build -o kv-store.exe .

# Start HTTP server (default port 8081)
.\kv-store.exe

# Or start CLI mode
.\kv-store.exe -mode cli
```

---

## Project Structure

```
KV-STORE/
├── main.go          # Entry point, flags, startup replay
├── store.go         # Core store, WAL, replay, compaction
├── cli.go           # Interactive command-line interface
├── http_server.go   # HTTP JSON API routes
├── store_test.go    # Unit tests
├── go.mod           # Go module definition
├── data.log         # Write-ahead log (created at runtime)
└── README.md        # This file
```

---

## How It Works

### Architecture

```
┌─────────────┐     ┌─────────────┐
│  CLI / HTTP │────▶│    Store    │
└─────────────┘     │  (in-memory │
                    │    map)     │
                    └──────┬──────┘
                           │
                    append + fsync
                           │
                           ▼
                    ┌─────────────┐
                    │  data.log   │  ← write-ahead log (WAL)
                    └─────────────┘
```

On **startup**, the program reads `data.log` line by line and replays every operation to rebuild the in-memory map.

On **write** (`SET` / `DELETE`), the operation is:
1. Applied to the in-memory map
2. Serialized to a plain-text line
3. Appended to `data.log`
4. Flushed to disk with `fsync`
5. Optionally compacted if thresholds are reached

### Log Format

Each line in `data.log` is one operation:

| Operation | Format | Example |
|-----------|--------|---------|
| Set       | `SET <key> <value>` | `SET name kiran` |
| Delete    | `DELETE <key>` | `DELETE name` |

Values can contain spaces — everything after the key is treated as the value:

```
SET greeting hello world
```

replays as key=`greeting`, value=`hello world`.

---

## Build Steps (20-Step Guide)

This project follows a progressive build guide. Each step maps to code in the repo:

| Step | What was built | Where |
|------|----------------|-------|
| 1 | In-memory map + add/read functions | `store.go` — `add()`, `read()` |
| 2 | SET operation (overwrite existing keys) | `store.go` — `Set()` |
| 3 | GET operation | `store.go` — `Get()` |
| 4 | Handle missing keys (no crash) | `store.go` — `Get()` returns `(value, false)` |
| 5 | DELETE operation | `store.go` — `Delete()` |
| 6 | List all keys | `store.go` — `ListKeys()` |
| 7 | Interactive CLI loop | `cli.go` — `runCLI()` |
| 8 | Serialize one entry to a text line | `store.go` — `serializeSet()`, `serializeDelete()` |
| 9 | Append to log file on SET/DELETE | `store.go` — `appendToLog()` |
| 10 | Read log file line by line | `store.go` — `readLogLines()` |
| 11 | Replay log into memory | `store.go` — `Replay()` |
| 12 | Auto-replay on startup | `main.go` — calls `store.Replay()` before serving |
| 13 | fsync after every write | `store.go` — `f.Sync()` in `appendToLog()` |
| 14 | Manual crash recovery test | See [Crash Recovery Test](#crash-recovery-test) |
| 15 | Skip corrupted last line | `store.go` — `Replay()` skips malformed last line |
| 16 | Compaction (latest value per key) | `store.go` — `compactLog()` |
| 17 | Atomic swap via temp file + rename | `store.go` — writes `data.log.tmp`, then `os.Rename()` |
| 18 | Auto-compaction trigger | `store.go` — `maybeCompact()` after 50 writes or 64 KB |
| 19 | HTTP routes `/set`, `/get`, `/delete` | `http_server.go` |
| 20 | End-to-end HTTP + restart test | See [HTTP API](#http-api) |

---

## Command-Line Flags

| Flag | Default | Description |
|------|---------|-------------|
| `-mode` | `http` | Run mode: `http` or `cli` |
| `-addr` | `:8081` | HTTP listen address (http mode only) |
| `-log` | `data.log` | Path to the write-ahead log file |

### Examples

```powershell
# HTTP on default port 8081
.\kv-store.exe

# HTTP on a custom port
.\kv-store.exe -mode http -addr :9090

# CLI with a custom log file
.\kv-store.exe -mode cli -log mydata.log
```

Port **8081** is used by default to avoid conflicting with the Java HTTP server in `HTTP-SERVER/` (port 8080).

---

## CLI Usage

Start CLI mode:

```powershell
.\kv-store.exe -mode cli
```

Available commands:

| Command | Description | Example |
|---------|-------------|---------|
| `SET <key> <value>` | Store or overwrite a key | `SET name kiran` |
| `GET <key>` | Read a value | `GET name` |
| `DELETE <key>` | Remove a key | `DELETE name` |
| `KEYS` | List all stored keys | `KEYS` |
| `QUIT` / `EXIT` | Exit the program | `QUIT` |

### CLI Example Session

```
Mini KV Store (commands: SET, GET, DELETE, KEYS, QUIT)
> SET name kiran
OK
> SET city mumbai
OK
> GET name
kiran
> KEYS
name, city
> DELETE city
OK
> GET city
NOT FOUND
> QUIT
bye
```

---

## HTTP API

Start the server:

```powershell
.\kv-store.exe -mode http -addr :8081
```

All responses are JSON.

### `GET /set?key=<key>&value=<value>`

Store or overwrite a key-value pair.

**Methods:** `GET`, `POST`

**Example (PowerShell):**

```powershell
Invoke-RestMethod "http://localhost:8081/set?key=name&value=kiran"
```

**Response (200 OK):**

```json
{
  "status": "OK",
  "key": "name",
  "value": "kiran"
}
```

---

### `GET /get?key=<key>`

Retrieve a value by key.

**Methods:** `GET`

**Example:**

```powershell
Invoke-RestMethod "http://localhost:8081/get?key=name"
```

**Response (200 OK):**

```json
{
  "key": "name",
  "value": "kiran"
}
```

**Response (404 Not Found):**

```json
{
  "error": "not found",
  "key": "missing"
}
```

---

### `GET /delete?key=<key>`

Remove a key from the store.

**Methods:** `GET`, `DELETE`

**Example:**

```powershell
Invoke-RestMethod "http://localhost:8081/delete?key=name"
```

**Response (200 OK):**

```json
{
  "status": "OK",
  "key": "name"
}
```

**Response (404 Not Found):**

```json
{
  "error": "not found",
  "key": "missing"
}
```

---

### `GET /keys`

List all keys currently in the store.

**Methods:** `GET`

**Example:**

```powershell
Invoke-RestMethod "http://localhost:8081/keys"
```

**Response (200 OK):**

```json
{
  "keys": ["name", "city"]
}
```

---

### HTTP Error Responses

| Status | When |
|--------|------|
| `400 Bad Request` | Missing required query parameter |
| `404 Not Found` | Key does not exist |
| `405 Method Not Allowed` | Wrong HTTP method for the route |
| `500 Internal Server Error` | Disk write or compaction failure |

---

## Durability & Crash Recovery

Every successful `SET` or `DELETE` follows this sequence:

1. Update the in-memory map
2. Append a serialized line to `data.log`
3. Call `fsync` to force the OS to flush data to disk
4. Return success to the caller

If the process is killed mid-write (power loss, `Stop-Process -Force`, Task Manager), the last line in the log may be incomplete. On restart:

- Valid lines are replayed in order
- The **last line** is skipped if it is malformed (Step 15)
- All other valid data is restored

### Crash Recovery Test

```powershell
# 1. Start the server
.\kv-store.exe -mode http -addr :8081

# 2. Store some data
Invoke-RestMethod "http://localhost:8081/set?key=name&value=kiran"
Invoke-RestMethod "http://localhost:8081/set?key=city&value=mumbai"

# 3. Force-kill the process (simulate a crash — do NOT use Ctrl+C)
Stop-Process -Name kv-store -Force

# 4. Restart the server
.\kv-store.exe -mode http -addr :8081

# 5. Confirm data survived
Invoke-RestMethod "http://localhost:8081/get?key=name"
# → {"key":"name","value":"kiran"}
```

You can also inspect the log directly:

```powershell
Get-Content data.log
```

Example output:

```
SET name kiran
SET city mumbai
DELETE city
```

---

## Log Compaction

Over time, repeated `SET` operations on the same key create redundant log entries. Compaction solves this by:

1. Reading the current in-memory state (latest value per key)
2. Writing one `SET` line per key to `data.log.tmp`
3. Flushing and closing the temp file
4. Atomically renaming `data.log.tmp` → `data.log`

Compaction runs automatically when **either** threshold is reached:

| Threshold | Value | Constant |
|-----------|-------|----------|
| Write count | 50 writes since last compaction | `compactAfterWrites` |
| Log file size | 64 KB | `compactAfterBytes` |

These constants are defined at the top of `store.go` and can be adjusted.

---

## Testing

Run the full test suite:

```powershell
go test ./...
```

Tests cover:

| Test | What it verifies |
|------|------------------|
| `TestStoreBasicOperations` | SET, GET, DELETE, missing key handling |
| `TestReplayAndDurability` | Log replay restores data after restart |
| `TestSkipCorruptedLastLine` | Malformed last line is skipped, valid data restored |
| `TestCompaction` | Compacted log replays to latest values only |
| `TestParseLogLine` | Log line parsing, including values with spaces |

Run with verbose output:

```powershell
go test -v ./...
```

---

## Concurrency

The store uses a `sync.RWMutex`:

- **Write operations** (`Set`, `Delete`, `Replay`, compaction) take an exclusive lock
- **Read operations** (`Get`, `ListKeys`) take a shared read lock

This allows multiple concurrent HTTP `GET` requests while writes are serialized.

---

## Limitations

This is a learning project, not production-ready:

- **No authentication** — anyone who can reach the port can read/write/delete
- **All data in memory** — very large datasets will consume RAM
- **Single node** — no replication or clustering
- **No TTL/expiry** — keys live until explicitly deleted
- **Key/value strings only** — no nested objects or binary data
- **No transactions** — operations are individual, not atomic batches
- **Compaction pauses writes** — compaction holds the write lock briefly

---

## Related Projects

| Project | Location | Description |
|---------|----------|-------------|
| HTTP Server (Java) | `../HTTP-SERVER/` | Raw-socket HTTP server built step-by-step in Java |

This KV store listens on port **8081** so it can run alongside the Java HTTP server on port **8080**.

---

## License

Part of the **BUILD OWN XYZ** learning repository.
