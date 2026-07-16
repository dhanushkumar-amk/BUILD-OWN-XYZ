# Step 5: Basic Routing

**Status:** Done

## Goal

Separate "parsing/networking" from "what each endpoint does."

## Build

- Create a lookup table: `(method, path)` → handler function
- Each handler returns a hardcoded response string (status + body)
- Return `404 Not Found` for unmatched routes

## Suggested structure

```java
interface Handler {
    String handle(HttpRequest request);
}

Map<String, Handler> routes = new HashMap<>();
routes.put("GET /", homeHandler);
routes.put("GET /about", aboutHandler);
```

## Example routes

| Method | Path    | Response body      |
|--------|---------|--------------------|
| GET    | `/`     | `Welcome home`     |
| GET    | `/about`| `About this server`|
| *      | *       | `404 Not Found`    |

## Test

```powershell
curl.exe localhost:8080/
curl.exe localhost:8080/about
curl.exe localhost:8080/missing
```

## Pass criteria

- `/` and `/about` return different bodies
- `/missing` returns `404` with appropriate status line
- Parsing still works (logged to console)

## Milestone

After Step 5 you have a **working single-threaded HTTP server with routing**.

## Next

Step 6 — accept loop for multiple sequential clients.
