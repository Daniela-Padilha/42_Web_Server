#include "../inc/CGI.hpp"

CGI::CGI(){}

CGI::CGI(const CGI& other) {
    *this = other;
}

CGI& CGI::operator=(const CGI& other) {
    if (this != &other) {
        env_ = other.env_;
    }
    return *this;
}

CGI::~CGI(){}

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

        // Execute the CGI script here (e.g., using execve)
        // execve("/path/to/cgi/script", NULL, env_.data());
    }
    else {
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
    }
}