#include "../inc/CGI.hpp"

CGI::CGI(const HTTPRequest& request, const RouteConfig& route)
    : _request(request), _route(route) {}

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
    std::string request_target = _request.get_request_target();

    std::string script_name = request_target; // Setting it to the full request target as a default value, in case there is no query string
    std::string query_string = "";
    size_t  query_pos = request_target.find('?');
    if (query_pos != std::string::npos) {
        script_name = request_target.substr(0, query_pos);
        query_string = request_target.substr(query_pos + 1);
    }
    
    // RFC 3875 required CGI environment variables
    _env.push_back("GATEWAY_INTERFACE=CGI/1.1");
    _env.push_back("SERVER_PROTOCOL=" + _request.get_protocol());
    _env.push_back("SERVER_NAME=" + _request.get_header_value("host"));
    _env.push_back("REQUEST_METHOD=" + _request.get_method());
    _env.push_back("SCRIPT_NAME=" + script_name);
    _env.push_back("PATH_INFO=" + request_target);
    _env.push_back("QUERY_STRING=" + query_string);
    _env.push_back("CONTENT_TYPE=" + _request.get_header_value("content-type"));
    _env.push_back("CONTENT_LENGTH=" + _request.get_header_value("content-length"));
    
    // php-cgi specific
    _env.push_back("SCRIPT_FILENAME=" + _getScriptPath());
    _env.push_back("REDIRECT_STATUS=200");
}

std::string CGI::_getScriptPath() const {
    std::string request_target = _request.get_request_target();

    size_t  query_pos = request_target.find('?');
    if (query_pos != std::string::npos) {
        request_target = request_target.substr(0, query_pos);
    }
    return _route.root + request_target;
}

void    CGI::_handleTimeout(pid_t pid) {
    int status;
    int elapsed = 0;

    while (elapsed < 5) {
        if (waitpid(pid, &status, WNOHANG) == pid) {
            return;
        }
        sleep(1);
        elapsed++;
    }

    // 5 seconds have passed and the child process is still running, kill it
    kill(pid, SIGKILL);         // force kill the script
    waitpid(pid, &status, 0);   // clean up zombie process
}

std::string CGI::execute() {
    _setEnvironment();

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

        std::vector<char*> env_ptrs;
        for (size_t i = 0; i < _env.size(); ++i) {
            env_ptrs.push_back(const_cast<char*>(_env[i].c_str()));
        }
        env_ptrs.push_back(NULL);

        std::string script_path = _getScriptPath();
        std::string script_dir = script_path.substr(0, script_path.find_last_of('/'));
        chdir(script_dir.c_str());

        char* argv[] = {
            const_cast<char*>(_route.cgi_path.c_str()),
            const_cast<char*>(script_path.c_str()),
            NULL
        };

        execve(_route.cgi_path.c_str(), argv, &env_ptrs[0]);
        exit(1);
    }
    else {
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);

        // write POST body to child stdin
        if (_request.get_method() == "POST") {
            const std::string& body = _request.get_body();
            write(stdin_pipe[1], body.c_str(), body.size());
        }
        close(stdin_pipe[1]); // signal EOF to child

        std::string output;
        char buffer[1024];
        ssize_t bytes_read;
        while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer))) > 0) {
            output.append(buffer, bytes_read);
        }
        close(stdout_pipe[0]);

        // wait for child process to finish, with a timeout to prevent hanging
        _handleTimeout(pid);

        return output;
    }
}