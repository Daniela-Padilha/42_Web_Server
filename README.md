_This project has been created as part of the 42 curriculum by abessa-m, ddo-carm, rjesus-d_

# Description

This project implements a fully functional HTTP web server in C++.

A **web server** stores, processes, and delivers web pages to clients. The client-server communication occurs through **HTTP** (Hypertext Transfer Protocol), a communication protocol that supports hypertext functionality (hyperlinks to other resources like web pages) and enables clients to send data (like uploading files or submitting forms).

The server implements **CGI** (Common Gateway Interface), an interface specification that facilitates communication between web servers and external databases or information sources. It acts as middleware, allowing web servers to interact with applications that process data and send back responses.

## Goal

Implement a fully functional HTTP server that handles multiple simultaneous connections, parses HTTP requests, serves static files, and supports CGI execution. Learn about sockets, HTTP protocol, and server architecture.

## Features

- **Multi-connection handling** - Handles multiple simultaneous client connections using select/poll
- **HTTP request parsing** - Parses HTTP/1.1 requests according to RFC 7230 and RFC 7231
- **Static file serving** - Serves static content (HTML, CSS, JS, images, etc.)
- **CGI execution** - Supports Common Gateway Interface for dynamic content
- **Configuration-based** - Customizable via configuration file

# Instructions

## Build

```bash
make
```

## Run

```bash
# Run with default config (config/default.conf)
./webserver

# Run with custom config
./webserver your_config.conf
```

## Configuration

The server uses a configuration file to define:
- Server port and host
- Server names (virtual hosts)
- Route definitions (URI to file/CGI mapping)
- Error pages
- Maximum client body size

See `config/default.conf` for an example configuration.

## Test

```bash
# Run tests
make test

# Clean build artifacts
make clean
```

## Usage Examples

```bash
# Test the server after running it
curl http://localhost:8080/

# Test a specific route
curl http://localhost:8080/index.html

# Test CGI endpoint (if configured)
curl http://localhost:8080/cgi-bin/script.py
```

# Resources

- **RFC 7230**: https://tools.ietf.org/html/rfc7230 (HTTP/1.1 Message Syntax and Routing)
- **RFC 7231**: https://tools.ietf.org/html/rfc7231 (HTTP/1.1 Semantics and Content)
- **NGINX**: Open source HTTP web server designed for maximum performance and stability
- **Telnet**: Network protocol that allows remote access and control of another computer (useful for testing HTTP)
- **RFCs (Request for Comments)**: Public documents that describe technical standards, protocols, and rules that govern the Internet

## AI Usage

AI tools were used throughout the development process for:
- Research and understanding of HTTP protocol specifications
- Code debugging and troubleshooting
- Explaining complex concepts (sockets, CGI, HTTP parsing)
- Code review and optimization suggestions

AI was used as a learning aid and productivity tool, not for generating the core implementation without understanding.
