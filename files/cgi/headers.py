#!/usr/bin/env python3
"""CGI test script: outputs a custom HTTP header."""

import os

print("Content-Type: text/html")
print("X-Custom: test123")
print("")
print("<html><body>")
print("<h1>Custom Header Test</h1>")
print("<p>Method: " + os.environ.get("REQUEST_METHOD", "") + "</p>")
print("</body></html>")
