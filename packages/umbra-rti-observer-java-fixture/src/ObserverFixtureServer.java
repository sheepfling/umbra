import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayDeque;
import java.util.Arrays;
import java.util.Deque;
import java.util.HashMap;
import java.util.Map;

/** HTTP-only contract test double, not a Java RTI adapter or federation server.
 * No Python process, native bindings, Maven or third-party JSON library needed.
 * JSON bodies and request/status expectations are shared data-only fixtures.
 */
public final class ObserverFixtureServer {
    private record Reply(int status, byte[] body) {}

    private static String target(String path, String query) {
        if (query == null || query.isEmpty()) return path;
        String[] fields = query.split("&");
        Arrays.sort(fields);
        return path + "?" + String.join("&", fields);
    }

    private static Path inside(Path root, String child) throws IOException {
        Path result = root.resolve(child).normalize().toRealPath();
        if (!result.startsWith(root.toRealPath())) throw new IOException("Path outside fixture root");
        return result;
    }

    private static void send(HttpExchange exchange, int status, String type, byte[] body) throws IOException {
        exchange.getResponseHeaders().set("Content-Type", type);
        exchange.getResponseHeaders().set("Cache-Control", "no-store");
        exchange.getResponseHeaders().set("X-Umbra-Observer-Contract", "umbra-observer-contract-v1");
        exchange.getResponseHeaders().set("X-Umbra-Observer-Media-Type", "application/vnd.umbra.observer+json");
        exchange.sendResponseHeaders(status, body.length);
        try (var stream = exchange.getResponseBody()) { stream.write(body); }
    }

    private static void error(HttpExchange exchange, int status) throws IOException {
        send(exchange, status, "application/json", "{\"detail\":\"Fixture route not available\"}".getBytes(StandardCharsets.UTF_8));
    }

    public static void main(String[] args) throws Exception {
        if (args.length < 1 || args.length > 2)
            throw new IllegalArgumentException("Usage: ObserverFixtureServer FIXTURE_PROFILE_DIRECTORY [UI_DIST]");
        Path fixture = Path.of(args[0]).toRealPath();
        Path ui = args.length == 2 ? Path.of(args[1]).toRealPath() : null;
        Map<String, Deque<Reply>> replies = new HashMap<>();
        for (String line : Files.readAllLines(fixture.resolve("routes.tsv"), StandardCharsets.UTF_8)) {
            String[] fields = line.split("\t", -1);
            if (fields.length != 3) throw new IOException("Invalid fixture manifest row");
            String[] uri = fields[0].split("\\?", 2);
            String key = target(uri[0], uri.length == 2 ? uri[1] : null);
            replies.computeIfAbsent(key, ignored -> new ArrayDeque<>()).addLast(
                new Reply(Integer.parseInt(fields[1]), Files.readAllBytes(inside(fixture, fields[2]))));
        }
        HttpServer server = HttpServer.create(new InetSocketAddress("127.0.0.1", 0), 0);
        server.createContext("/", exchange -> {
            try {
                if (!exchange.getRequestMethod().equals("GET")) {
                    exchange.getResponseHeaders().set("Allow", "GET");
                    error(exchange, 405);
                    return;
                }
                String path = exchange.getRequestURI().getPath();
                Deque<Reply> queue = replies.get(target(path, exchange.getRequestURI().getRawQuery()));
                if (queue != null) {
                    Reply reply = queue.size() > 1 ? queue.removeFirst() : queue.getFirst();
                    send(exchange, reply.status(), "application/json", reply.body());
                } else if (ui != null && (path.equals("/") || path.startsWith("/assets/"))) {
                    try {
                        Path file = inside(ui, path.equals("/") ? "index.html" : path.substring(1));
                        String type = path.endsWith(".js") ? "text/javascript" : path.endsWith(".css") ? "text/css" : "text/html";
                        send(exchange, 200, type, Files.readAllBytes(file));
                    } catch (IOException error) { error(exchange, 404); }
                } else error(exchange, 404);
            } finally { exchange.close(); }
        });
        Runtime.getRuntime().addShutdownHook(new Thread(() -> server.stop(0)));
        server.start();
        System.out.println("OBSERVER_FIXTURE_URL=http://127.0.0.1:" + server.getAddress().getPort());
        System.out.flush();
    }
}
