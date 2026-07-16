# Step 13: Serve Static Files (Optional)

**Status:** Done

## Goal

Practical utility + a real security lesson (path traversal).

## Build

- Handler reads a file from disk based on request path
- Set `Content-Type` from file extension (`.html`, `.css`, `.png`, etc.)
- **Block path traversal** — reject paths containing `..`

```java
if (path.contains("..")) {
    return errorResponse(403, "Forbidden");
}
Path filePath = Paths.get("public", path).normalize();
if (!filePath.startsWith(publicDir)) {
    return errorResponse(403, "Forbidden");
}
```

## Project layout

```
HTTP-SERVER/
  public/
    index.html
    style.css
```

## Test

```powershell
# Should work
curl.exe localhost:8080/index.html

# Should be blocked
curl.exe "localhost:8080/../../etc/passwd"
```

Open `http://localhost:8080/index.html` in browser — page should render.

## Pass criteria

- Valid files served with correct `Content-Type`
- `../` traversal attempts return 403
- Missing files return 404

## Next

Step 14 — request logging.
