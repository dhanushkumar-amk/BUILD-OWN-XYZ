# Step 12: Path Parameters and Query Strings

**Status:** Done

## Goal

Support realistic API-style endpoints.

## Build

### Path parameters

Support route patterns like `/users/{id}`:

```
GET /users/42  →  id = "42"
```

### Query strings

Parse `?key=value&...` off the path:

```
GET /users/42?verbose=true  →  id="42", query={verbose=true}
```

## Suggested structure

```java
class HttpRequest {
    String method;
    String path;
    Map<String, String> pathParams;
    Map<String, String> queryParams;
    Map<String, String> headers;
    String body;
}
```

## Test

```powershell
curl.exe "localhost:8080/users/42?verbose=true"
```

**Expected handler receives:** `id=42`, `verbose=true`

## Pass criteria

- `/users/42` extracts `id=42`
- `?verbose=true&limit=10` parses both query params
- Unmatched paths still return 404

## Next

Step 13 — serve static files from disk.
