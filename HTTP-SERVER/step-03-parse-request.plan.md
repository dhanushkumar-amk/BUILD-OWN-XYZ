# Step 3: Parse the Incoming Request

**Status:** Done

## Goal

Confidently turn raw bytes into structured data.

## Build

- Read request line: `GET /path HTTP/1.1`
- Read headers line-by-line until blank line
- Store in a simple request object:
  - `method` (String)
  - `path` (String)
  - `headers` (Map<String, String>)
- Print parsed result to console instead of raw bytes

## Suggested structure

```java
class HttpRequest {
    String method;
    String path;
    Map<String, String> headers = new HashMap<>();
}
```

## Parsing logic

1. Read first line → split on spaces → `method`, `path`, `version`
2. Read subsequent lines until empty line → split on first `:` → header name/value
3. Trim whitespace from header values

## Test

```powershell
# Browser
# Open http://localhost:8080/some/path

# curl with custom path and header
curl.exe -v http://localhost:8080/test -H "X-Custom: hello"
```

Compare server console output against `curl -v` output. Parsed method, path, and headers must match.

## Pass criteria

- `GET /test HTTP/1.1` → method=`GET`, path=`/test`
- Headers like `Host`, `User-Agent`, `X-Custom` parsed correctly
- Still returns hardcoded 200 response from Step 2

## Next

Step 4 — read request body using `Content-Length`.
