import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class EchoServer {
    private static final List<Route> routes = new ArrayList<>();
    private static final Object logLock = new Object();

    static {
        routes.add(new Route("GET", "/", request -> "Welcome home"));
        routes.add(new Route("GET", "/about", request -> "About this server"));
        routes.add(new Route("GET", "/slow", request -> {
            try {
                Thread.sleep(3000);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            return "Slow response";
        }));
        routes.add(new Route("GET", "/users/{id}", request -> {
            String id = request.pathParams.get("id");
            String verbose = request.queryParams.getOrDefault("verbose", "false");
            return "User ID: " + id + " (verbose=" + verbose + ")";
        }));
    }

    public static void main(String[] args) throws Exception {
        ServerSocket serverSocket = new ServerSocket(8080);
        System.out.println("Listening on port 8080...");
        ExecutorService pool = Executors.newFixedThreadPool(4);

        // Register shutdown hook
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            System.out.println("\nShutting down server gracefully...");
            try {
                serverSocket.close();
            } catch (Exception e) {
                // ignore
            }
            pool.shutdown();
            try {
                if (!pool.awaitTermination(30, TimeUnit.SECONDS)) {
                    pool.shutdownNow();
                }
            } catch (InterruptedException e) {
                pool.shutdownNow();
                Thread.currentThread().interrupt();
            }
            System.out.println("Shutdown complete.");
        }));

        try {
            while (!serverSocket.isClosed()) {
                try {
                    Socket client = serverSocket.accept();
                    System.out.println("Client connected: " + client.getRemoteSocketAddress());
                    pool.submit(() -> handleClient(client));
                } catch (java.net.SocketException e) {
                    if (serverSocket.isClosed()) {
                        break;
                    }
                    System.err.println("Socket error: " + e.getMessage());
                }
            }
        } finally {
            if (!serverSocket.isClosed()) {
                serverSocket.close();
            }
            pool.shutdown();
        }
    }

    private static void handleClient(Socket client) {
        try {
            client.setSoTimeout(5000); // 5 seconds timeout
        } catch (Exception e) {
            System.err.println("Failed to set SO_TIMEOUT: " + e.getMessage());
        }

        try (InputStream in = client.getInputStream();
             OutputStream out = client.getOutputStream()) {
            
            boolean keepAlive = true;
            while (keepAlive) {
                long startTime = System.currentTimeMillis();
                HttpRequest request;
                try {
                    request = HttpRequest.parse(in);
                } catch (IllegalArgumentException e) {
                    sendError(out, 400, "Bad Request", false);
                    logRequest("UNKNOWN", "UNKNOWN", 400, System.currentTimeMillis() - startTime);
                    break;
                }

                if (request == null) {
                    break; // EOF
                }

                // Match route
                Handler handler = null;
                for (Route route : routes) {
                    Map<String, String> params = matchRoute(route.method, route.pattern, request.method, request.path);
                    if (params != null) {
                        handler = route.handler;
                        request.pathParams = params;
                        break;
                    }
                }

                int statusCode;
                if (handler != null) {
                    try {
                        String responseBody = handler.handle(request);
                        
                        // Check keep-alive
                        String connHeader = null;
                        for (String key : request.headers.keySet()) {
                            if (key.equalsIgnoreCase("Connection")) {
                                connHeader = request.headers.get(key);
                                break;
                            }
                        }
                        if (connHeader != null && connHeader.equalsIgnoreCase("close")) {
                            keepAlive = false;
                        }

                        String response =
                            "HTTP/1.1 200 OK\r\n" +
                            "Content-Length: " + responseBody.length() + "\r\n" +
                            "Content-Type: text/plain\r\n" +
                            "Connection: " + (keepAlive ? "keep-alive" : "close") + "\r\n" +
                            "\r\n" +
                            responseBody;

                        out.write(response.getBytes());
                        out.flush();
                        statusCode = 200;
                    } catch (Exception e) {
                        sendError(out, 500, "Internal Server Error", false);
                        statusCode = 500;
                        keepAlive = false;
                    }
                } else {
                    // Fallback to static files
                    try {
                        statusCode = serveStaticFile(request.path, out);
                        if (statusCode == 404) {
                            sendError(out, 404, "Not Found", keepAlive);
                        } else if (statusCode == 403) {
                            keepAlive = false;
                        }
                    } catch (Exception e) {
                        sendError(out, 500, "Internal Server Error", false);
                        statusCode = 500;
                        keepAlive = false;
                    }
                }

                long duration = System.currentTimeMillis() - startTime;
                logRequest(request.method, request.path, statusCode, duration);
            }
        } catch (java.net.SocketTimeoutException e) {
            System.out.println("Connection idle timeout occurred.");
        } catch (Exception e) {
            System.err.println("Exception in handleClient: " + e.getMessage());
        } finally {
            try {
                client.close();
            } catch (Exception e) {
                // ignore
            }
            System.out.println("Client disconnected.\n");
        }
    }

    private static Map<String, String> matchRoute(String routeMethod, String routePattern, String requestMethod, String requestPath) {
        if (!routeMethod.equalsIgnoreCase(requestMethod)) {
            return null;
        }
        
        String normalizedPattern = trimSlashes(routePattern);
        String normalizedPath = trimSlashes(requestPath);
        
        String[] patternSegments = normalizedPattern.isEmpty() ? new String[0] : normalizedPattern.split("/");
        String[] pathSegments = normalizedPath.isEmpty() ? new String[0] : normalizedPath.split("/");
        
        if (patternSegments.length != pathSegments.length) {
            return null;
        }
        
        Map<String, String> pathParams = new HashMap<>();
        for (int i = 0; i < patternSegments.length; i++) {
            String pSeg = patternSegments[i];
            String rSeg = pathSegments[i];
            if (pSeg.startsWith("{") && pSeg.endsWith("}")) {
                String paramName = pSeg.substring(1, pSeg.length() - 1);
                pathParams.put(paramName, rSeg);
            } else if (!pSeg.equals(rSeg)) {
                return null;
            }
        }
        return pathParams;
    }

    private static String trimSlashes(String s) {
        if (s.startsWith("/")) {
            s = s.substring(1);
        }
        if (s.endsWith("/")) {
            s = s.substring(0, s.length() - 1);
        }
        return s;
    }

    private static int serveStaticFile(String path, OutputStream out) throws Exception {
        // Block path traversal
        if (path.contains("..")) {
            sendError(out, 403, "Forbidden", false);
            return 403;
        }

        String relativePath = path.startsWith("/") ? path.substring(1) : path;
        if (relativePath.isEmpty() || relativePath.endsWith("/")) {
            relativePath += "index.html";
        }

        java.nio.file.Path publicDir = java.nio.file.Paths.get("public").toAbsolutePath().normalize();
        java.nio.file.Path filePath = publicDir.resolve(relativePath).normalize();

        // Security check: ensure path is within the public directory
        if (!filePath.startsWith(publicDir)) {
            sendError(out, 403, "Forbidden", false);
            return 403;
        }

        java.io.File file = filePath.toFile();
        if (!file.exists() || file.isDirectory()) {
            return 404;
        }

        byte[] fileBytes = java.nio.file.Files.readAllBytes(filePath);
        String contentType = getContentType(relativePath);

        String responseHeaders =
            "HTTP/1.1 200 OK\r\n" +
            "Content-Length: " + fileBytes.length + "\r\n" +
            "Content-Type: " + contentType + "\r\n" +
            "Connection: keep-alive\r\n" +
            "\r\n";

        out.write(responseHeaders.getBytes());
        out.write(fileBytes);
        out.flush();
        return 200;
    }

    private static String getContentType(String path) {
        if (path.endsWith(".html") || path.endsWith(".htm")) return "text/html";
        if (path.endsWith(".css")) return "text/css";
        if (path.endsWith(".js")) return "application/javascript";
        if (path.endsWith(".png")) return "image/png";
        if (path.endsWith(".jpg") || path.endsWith(".jpeg")) return "image/jpeg";
        if (path.endsWith(".gif")) return "image/gif";
        return "application/octet-stream";
    }

    private static void sendError(OutputStream out, int statusCode, String message, boolean keepAlive) throws Exception {
        String response =
            "HTTP/1.1 " + statusCode + " " + message + "\r\n" +
            "Content-Length: " + message.length() + "\r\n" +
            "Content-Type: text/plain\r\n" +
            "Connection: " + (keepAlive ? "keep-alive" : "close") + "\r\n" +
            "\r\n" +
            message;
        out.write(response.getBytes());
        out.flush();
    }

    private static void logRequest(String method, String path, int statusCode, long duration) {
        synchronized (logLock) {
            System.out.println(method + " " + path + " " + statusCode + " " + duration + "ms");
        }
    }

    private static String readLine(InputStream in) throws Exception {
        StringBuilder sb = new StringBuilder();
        int c;
        while ((c = in.read()) != -1) {
            if (c == '\r') {
                continue;
            }
            if (c == '\n') {
                break;
            }
            sb.append((char) c);
        }
        if (c == -1 && sb.length() == 0) {
            return null;
        }
        return sb.toString();
    }

    interface Handler {
        String handle(HttpRequest request) throws Exception;
    }

    static class Route {
        String method;
        String pattern;
        Handler handler;

        Route(String method, String pattern, Handler handler) {
            this.method = method;
            this.pattern = pattern;
            this.handler = handler;
        }
    }

    static class HttpRequest {
        String method;
        String path;
        Map<String, String> pathParams = new HashMap<>();
        Map<String, String> queryParams = new HashMap<>();
        Map<String, String> headers = new HashMap<>();
        String body;

        public static HttpRequest parse(InputStream in) throws Exception {
            String requestLine = readLine(in);
            if (requestLine == null) {
                return null;
            }
            if (requestLine.isEmpty()) {
                throw new IllegalArgumentException("Empty request line");
            }

            HttpRequest request = new HttpRequest();
            String[] parts = requestLine.split(" ");
            if (parts.length < 3) {
                throw new IllegalArgumentException("Invalid request line: " + requestLine);
            }
            request.method = parts[0];
            String rawPath = parts[1];

            // Parse path and query parameters
            int qMark = rawPath.indexOf('?');
            if (qMark != -1) {
                request.path = rawPath.substring(0, qMark);
                String queryStr = rawPath.substring(qMark + 1);
                for (String param : queryStr.split("&")) {
                    String[] kv = param.split("=", 2);
                    if (kv.length == 2) {
                        request.queryParams.put(kv[0], kv[1]);
                    } else if (kv.length == 1) {
                        request.queryParams.put(kv[0], "");
                    }
                }
            } else {
                request.path = rawPath;
            }

            String line;
            while ((line = readLine(in)) != null && !line.isEmpty()) {
                int colonIndex = line.indexOf(':');
                if (colonIndex == -1) {
                    throw new IllegalArgumentException("Malformed header line: " + line);
                }
                String name = line.substring(0, colonIndex).trim();
                String value = line.substring(colonIndex + 1).trim();
                request.headers.put(name, value);
            }

            // Read the body if Content-Length header is present
            String clStr = null;
            for (String key : request.headers.keySet()) {
                if (key.equalsIgnoreCase("Content-Length")) {
                    clStr = request.headers.get(key);
                    break;
                }
            }

            int contentLength = 0;
            if (clStr != null) {
                try {
                    contentLength = Integer.parseInt(clStr);
                } catch (NumberFormatException e) {
                    contentLength = 0;
                }
            }

            if (contentLength > 0) {
                byte[] bodyBytes = new byte[contentLength];
                int totalRead = 0;
                while (totalRead < contentLength) {
                    int n = in.read(bodyBytes, totalRead, contentLength - totalRead);
                    if (n == -1) break;
                    totalRead += n;
                }
                request.body = new String(bodyBytes, 0, totalRead);
            }

            return request;
        }
    }
}
