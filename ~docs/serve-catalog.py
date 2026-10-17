"""Simple HTTP server that serves the icon catalog with proper asset paths."""
import http.server
import os
import sys

PORT = 8169
UI_DIR = os.path.join(os.path.dirname(__file__), '..', 'UI')
DOCS_DIR = os.path.dirname(__file__)

class CatalogHandler(http.server.SimpleHTTPRequestHandler):
    def translate_path(self, path):
        # /assets/X.png -> UI/Assets/X.png
        if path.startswith('/assets/'):
            return os.path.join(UI_DIR, 'Assets', path[8:])
        # Everything else -> ~docs/
        return os.path.join(DOCS_DIR, path.lstrip('/'))

    def log_message(self, format, *args):
        pass  # Suppress logging

if __name__ == '__main__':
    os.chdir(DOCS_DIR)
    with http.server.HTTPServer(('127.0.0.1', PORT), CatalogHandler) as httpd:
        print(f'Icon catalog: http://127.0.0.1:{PORT}/icon-catalog.html')
        sys.stdout.flush()
        httpd.serve_forever()
