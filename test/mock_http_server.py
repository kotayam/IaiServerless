#!/usr/bin/env python3
from http.server import HTTPServer, BaseHTTPRequestHandler
import sys

class MockHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        print(f"[Mock Server] Received GET {self.path}")
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        self.wfile.write(b"Hello from the IaiServerless Mock Server!\n")
        print("[Mock Server] Response sent.")

if __name__ == "__main__":
    port = 8000
    try:
        server = HTTPServer(('127.0.0.1', port), MockHandler)
        print(f"Mock HTTP Server running on http://127.0.0.1:{port}...")
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down Mock Server.")
        sys.exit(0)
