#!/usr/bin/env python3
"""CGI test script: returns a 404 status code via the Status header."""

print("Status: 404 Not Found")
print("Content-Type: text/html")
print("")
print("<html><body>")
print("<h1>CGI 404 Test</h1>")
print("<p>This page intentionally returns 404.</p>")
print("</body></html>")
