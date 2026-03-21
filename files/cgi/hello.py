#!/usr/bin/env python3
import os
import sys

method = os.environ.get("REQUEST_METHOD", "")
query = os.environ.get("QUERY_STRING", "")
host = os.environ.get("SERVER_NAME", "")

body_input = ""
if method == "POST":
    content_length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
    if content_length > 0:
        body_input = sys.stdin.read(content_length)

print("Content-Type: text/html")
print("")
print("<!DOCTYPE html>")
print("<html><head><title>CGI Hello</title></head><body>")
print("<h1>Hello from CGI!</h1>")
print("<pre>")
print("Method:       " + method)
print("Query string: " + query)
print("Host:         " + host)
if body_input:
    print("POST body:    " + body_input)
print("</pre>")
print("</body></html>")
