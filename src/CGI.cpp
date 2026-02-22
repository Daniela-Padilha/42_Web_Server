#include "../inc/CGI.hpp"

CGI::CGI(){}

CGI::CGI(const CGI& other) {
    *this = other;
}

CGI& CGI::operator=(const CGI& other) {
    if (this != &other) {
        _env = other._env;
    }
    return *this;
}

CGI::~CGI(){}

// Check RFC 3875 for CGI environment variables: https://datatracker.ietf.org/doc/html/rfc3875
void CGI::_setEnvironment() {
    // Query string is the part of the URL after the '?' character, if any
    std::string query_string = "";
    std::string request_target = _request.get_request_target();
    size_t query_pos = request_target.find('?');
    if (query_pos != std::string::npos) {
        query_string = request_target.substr(query_pos + 1);
    }

    // Method used by the script to process the request
    _env.push_back("REQUEST_METHOD=" + _request.get_method());

    // The MIME type of the request body, if any
    _env.push_back("CONTENT_TYPE=" + _request.get_header_value("content-type"));
    
    // The length of the request body, if any
    _env.push_back("CONTENT_LENGTH=" + _request.get_header_value("content-length"));
    
    // The query string, if any
    _env.push_back("QUERY_STRING=" + query_string);
    
    //_env.push_back("SCRIPT_NAME=" + ...); the URL parh as the client requested it
    
    // The path to the resource being requested, relative to the document root
    _env.push_back("PATH_INFO=" + request_target);

    // The protocol version used by the client
    _env.push_back("SERVER_PROTOCOL=" + _request.get_protocol());

    // The server's hostname or IP address
    _env.push_back("SERVER_NAME=" + _request.get_header_value("host"));

    // php-cgi specifically requires it as a security measure
    _env.push_back("REDIRECT_STATUS=200");
}

std::string CGI::execute() {
    int stdin_pipe[2];
    int stdout_pipe[2];

    pipe(stdin_pipe);
    pipe(stdout_pipe);

    pid_t pid = fork();

    if (pid == 0) {
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);

        close(stdin_pipe[1]);
        close(stdout_pipe[0]);

        //...
    }
    else {
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
    }
}