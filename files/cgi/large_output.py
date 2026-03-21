#!/usr/bin/env python3
"""CGI test script: outputs ~100KB of data to test EOF-based content length."""

print("Content-Type: text/plain")
print("")

# Generate approximately 100KB of output (1000 lines of 100 chars each)
line = "A" * 99  # 99 chars + newline = 100 bytes per line
for i in range(1000):
    print(line)

print("CGI_LARGE_OUTPUT_END")
