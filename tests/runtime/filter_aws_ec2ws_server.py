#!/usr/bin/env python3

import sys
from http.server import BaseHTTPRequestHandler, HTTPServer


SHOULD_RETURN_TAGS = True


class HTTPHandler(BaseHTTPRequestHandler):
    def _send_response(self, status_code: int = 200, body: str = None):
        self.send_response(status_code)
        if body:
            self.send_header("Content-Type", "text/plain")
            self.send_header("Content-Length", len(body))
        self.end_headers()
        if body:
            self.wfile.write(body.encode("utf-8"))

    def do_GET(self):
        if self.path == "/latest/meta-data/instance-id/":
            self._send_response(body="i-0e66fc7f9809d7168")

        elif self.path == "/latest/meta-data/placement/availability-zone/":
            self._send_response(body="us-east-1a")

        elif self.path == "/latest/meta-data/tags/instance":
            if not SHOULD_RETURN_TAGS:
                # If there are 0 tags, AWS returns a 404.
                self._send_response(status_code=404)
            else:
                self._send_response(body="Name\nCUSTOMER_ID")

        elif self.path == "/latest/meta-data/tags/instance/Name":
            self._send_response(body="mwarzynski-fluentbit-dev")

        elif self.path == "/latest/meta-data/tags/instance/CUSTOMER_ID":
            self._send_response(body="70ec5c04-3a6e-11ed-a261-0242ac120002")

        else:
            self._send_response(status_code=500)

    def do_PUT(self):
        self._send_response(status_code=200)


def run():
    if len(sys.argv) == 2 and "no_tags" == sys.argv[1]:
        global SHOULD_RETURN_TAGS
        SHOULD_RETURN_TAGS = False
    httpd = HTTPServer(("", 8000), HTTPHandler)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()


if __name__ == "__main__":
    run()
