#!/usr/bin/env python3
"""CGI test script: prints PATH_INFO and working directory for verification."""

import os

print("Content-Type: text/plain")
print("")
print("PATH_INFO=" + os.environ.get("PATH_INFO", ""))
print("SCRIPT_NAME=" + os.environ.get("SCRIPT_NAME", ""))
print("QUERY_STRING=" + os.environ.get("QUERY_STRING", ""))
print("REQUEST_METHOD=" + os.environ.get("REQUEST_METHOD", ""))
print("SERVER_PROTOCOL=" + os.environ.get("SERVER_PROTOCOL", ""))
print("CWD=" + os.getcwd())
