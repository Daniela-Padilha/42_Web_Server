#!/usr/bin/env python3
"""CGI test script: hangs forever to test server timeout handling."""

import time

# Do not print any headers or body before sleeping.
# The server should kill this process after its timeout (5 seconds).
time.sleep(30)
print("Content-Type: text/html")
print("")
print("<html><body>Should never reach here</body></html>")
