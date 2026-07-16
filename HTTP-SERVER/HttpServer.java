import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;

public class HttpServer {
    public static void main(String[] args) throws Exception {
        ServerSocket serverSocket = new ServerSocket(8080);
        System.out.println("Listening on port 8080...");

        try {
            // 1. Accept one connection
            Socket client = serverSocket.accept();
            System.out.println("Client connected: " + client.getRemoteSocketAddress());

            try {
                InputStream in = client.getInputStream();
                OutputStream out = client.getOutputStream();

                // 2. Read whatever the client sent (we need to consume it, but we ignore it)
                byte[] buffer = new byte[1024];
                int bytesRead = in.read(buffer);
                System.out.println("Received " + bytesRead + " bytes (ignoring content)");

                // 3. Build a hardcoded HTTP response
                String body = "<html><body><h1>Hello from my HTTP server!</h1><p>Step 2 works.</p></body></html>";
                String response = "HTTP/1.1 200 OK\r\n"
                        + "Content-Type: text/html\r\n"
                        + "Content-Length: " + body.length() + "\r\n"
                        + "\r\n"  // blank line = end of headers
                        + body;

                // 4. Send it back
                out.write(response.getBytes());
                out.flush();
                System.out.println("Response sent!");

            } finally {
                client.close();
            }

            System.out.println("Client disconnected.");
        } finally {
            serverSocket.close();
        }
    }
}
