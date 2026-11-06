from http.server import HTTPServer, SimpleHTTPRequestHandler
from urllib.parse import parse_qs, urlparse
from pathlib import Path
import subprocess
import sys
import webbrowser
from functools import partial

# This server is the bridge between the web page and the C program.
# The next lines locate the files inside the packaged Windows application.
FOLDER = Path(getattr(sys, '_MEIPASS', Path(__file__).resolve().parent))
# Data exists only in this variable while the server is running.
event_data = ''

class Handler(SimpleHTTPRequestHandler):
    # GET requests open the page, its files, or the list/history/search of events.
    def do_GET(self):
        parsed = urlparse(self.path)
        if parsed.path == '/api/list':
            self.run_c(['list'])
        elif parsed.path == '/api/history':
            self.run_c(['history'])
        elif parsed.path == '/api/search':
            params = parse_qs(parsed.query)
            q = params.get('q', params.get('query', ['']))[0].strip()
            if not q:
                self.reply('Search query cannot be empty.', 400)
                return
            if len(q.encode('utf-8')) > 79 or any(c in q for c in '|\r\n'):
                self.reply('Please enter a valid search query (max 79 chars, no | or newlines).', 400)
                return
            self.run_c(['search', q])
        elif parsed.path in ('/', '/index.html', '/style.css', '/app.js'):
            super().do_GET()
        else:
            self.send_error(404)

    # POST requests send form details (add, register, cancel, or search).
    def do_POST(self):
        # Accept requests from local origins or direct programmatic requests
        origin = self.headers.get('Origin')
        allowed_origins = ('http://127.0.0.1:8000', 'http://localhost:8000')
        if origin is not None and origin not in allowed_origins:
            self.send_error(403)
            return

        parsed = urlparse(self.path)
        # Each operation needs a specific set of fields from its form.
        fields_for = {
            '/api/add': ['name', 'category', 'capacity'],
            '/api/register': ['event', 'id', 'name', 'department'],
            '/api/cancel': ['event', 'id'],
            '/api/search': ['query']
        }
        if parsed.path not in fields_for:
            self.send_error(404)
            return

        size = int(self.headers.get('Content-Length', '0'))
        if size > 4096:
            self.send_error(413)
            return
        data = parse_qs(self.rfile.read(size).decode('utf-8'))
        values = [data.get(key, [''])[0].strip() for key in fields_for[parsed.path]]

        # Check that values fit the character arrays used in the C program.
        limits = {'name': 79, 'category': 39, 'capacity': 3, 'event': 3, 'id': 39, 'department': 79, 'query': 79}
        if any(not value or len(value.encode('utf-8')) > limits[key] or any(c in value for c in '|\r\n')
               for key, value in zip(fields_for[parsed.path], values)):
            self.reply('Please enter valid values; | and new lines are not allowed.', 400)
            return
        for key, value in zip(fields_for[parsed.path], values):
            if key in ('event', 'capacity') and (not value.isascii() or not value.isdigit()):
                self.reply('Event ID and capacity must be whole numbers.', 400)
                return

        action = parsed.path.split('/')[-1]
        self.run_c([action] + values)

    # Run the compiled C program and send its output back to the browser.
    def run_c(self, args):
        global event_data
        try:
            result = subprocess.run([str(FOLDER / 'event_backend.exe')] + args,
                                    input=event_data, capture_output=True, text=True, encoding='utf-8', timeout=5, creationflags=subprocess.CREATE_NO_WINDOW)
            if result.returncode == 0:
                event_data = result.stderr # Updated event data from C, kept only in memory.
            self.reply(result.stdout if result.returncode == 0 else 'The C program could not complete the request.',
                       200 if result.returncode == 0 else 500)
        except (OSError, subprocess.TimeoutExpired):
            self.reply('Could not run the C program. Compile it first.', 500)

    # Send a plain-text response which JavaScript can show on the page.
    def reply(self, text, status=200):
        body = text.encode('utf-8')
        self.send_response(status)
        self.send_header('Content-Type', 'text/plain; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

# Start a local server and open the page in the default browser.
server = HTTPServer(('127.0.0.1', 8000), partial(Handler, directory=str(FOLDER)))
print('College Event Registration is running. Keep this window open.')
print('If the page does not open, visit http://127.0.0.1:8000')
print('Close this window when you are finished.')
webbrowser.open('http://127.0.0.1:8000')
server.serve_forever()




