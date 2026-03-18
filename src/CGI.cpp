#include "../inc/CGI.hpp"

CGI::CGI(const HTTPRequest &request, const RouteConfig &route) :
	_request(request),
	_route(route)
{
}

CGI::CGI(const CGI &other) : _request(other._request), _route(other._route)
{
	*this = other;
}

CGI &CGI::operator=(const CGI &other)
{
	if (this != &other)
	{
		_env = other._env;
	}
	return *this;
}

CGI::~CGI()
{
}

// Check RFC 3875 for CGI environment variables:
// https://datatracker.ietf.org/doc/html/rfc3875
void CGI::_setEnvironment()
{
	std::string request_target = _request.get_request_target();

	std::string script_name
		= request_target; // Setting it to the full request target as a default
						  // value, in case there is no query string
	std::string query_string = "";
	size_t		query_pos	 = request_target.find('?');
	if (query_pos != std::string::npos)
	{
		script_name	 = request_target.substr(0, query_pos);
		query_string = request_target.substr(query_pos + 1);
	}

	// RFC 3875 required CGI environment variables
	_env.push_back("GATEWAY_INTERFACE=CGI/1.1");
	_env.push_back("SERVER_PROTOCOL=" + _request.get_protocol());
	_env.push_back("SERVER_NAME=" + _request.get_header_value("host"));
	_env.push_back("REQUEST_METHOD=" + _request.get_method());
	_env.push_back("SCRIPT_NAME=" + script_name);
	_env.push_back("PATH_INFO=" + request_target);
	_env.push_back("REQUEST_URI=" + request_target);
	_env.push_back("QUERY_STRING=" + query_string);
	_env.push_back("CONTENT_TYPE=" + _request.get_header_value("content-type"));
	_env.push_back("CONTENT_LENGTH="
				   + _request.get_header_value("content-length"));

	// Forward all HTTP headers as HTTP_* env vars (RFC 3875 4.1.18)
	const std::map<std::string, std::string> &hdrs = _request.get_headers();
	for (std::map<std::string, std::string>::const_iterator it = hdrs.begin();
		 it != hdrs.end();
		 ++it)
	{
		// Skip headers already handled as their own CGI vars
		if (it->first == "content-type" || it->first == "content-length")
			continue;
		// Convert header name: lowercase to uppercase, '-' to '_'
		std::string env_name = "HTTP_";
		for (size_t i = 0; i < it->first.size(); ++i)
		{
			char c = it->first[i];
			if (c == '-')
				env_name += '_';
			else
				env_name += static_cast<char>(
					std::toupper(static_cast<unsigned char>(c)));
		}
		_env.push_back(env_name + "=" + it->second);
	}

	// php-cgi specific
	_env.push_back("SCRIPT_FILENAME=" + _getScriptPath());
	_env.push_back("REDIRECT_STATUS=200");
}

std::string CGI::_getScriptPath() const
{
	std::string request_target = _request.get_request_target();

	// Strip query string
	size_t		query_pos = request_target.find('?');
	if (query_pos != std::string::npos)
	{
		request_target = request_target.substr(0, query_pos);
	}

	// Strip the route prefix so the script path is relative to route.root.
	// e.g. route.path="/cgi-bin", request_target="/cgi-bin/hello.py"
	//      → strip_prefix → "/hello.py"
	//      route.root + "/hello.py" = "./files/cgi/hello.py"
	const std::string &prefix = _route.path;
	if (!prefix.empty() && prefix != "/"
		&& request_target.compare(0, prefix.size(), prefix) == 0)
	{
		request_target = request_target.substr(prefix.size());
		if (request_target.empty() || request_target[0] != '/')
		{
			request_target = "/" + request_target;
		}
	}

	return _route.root + request_target;
}

std::string CGI::execute()
{
	_setEnvironment();

	int stdin_pipe[2];
	int stdout_pipe[2];

	if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0)
	{
		eprint("CGI: pipe() failed");
		return "";
	}

	pid_t pid = fork();

	if (pid < 0)
	{
		eprint("CGI: fork() failed");
		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);
		return "";
	}

	if (pid == 0)
	{
		// Child process
		dup2(stdin_pipe[0], STDIN_FILENO);
		dup2(stdout_pipe[1], STDOUT_FILENO);

		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);

		std::vector<char *> env_ptrs;
		for (size_t i = 0; i < _env.size(); ++i)
		{
			env_ptrs.push_back(const_cast<char *>(_env[i].c_str()));
		}
		env_ptrs.push_back(NULL);

		std::string script_path = _getScriptPath();

		// Resolve both paths to absolute before chdir() changes CWD.
		// realpath() resolves symlinks and relative segments.
		char		resolved_script[4096];
		if (realpath(script_path.c_str(), resolved_script) != NULL)
		{
			script_path = resolved_script;
		}

		char		resolved_cgi[4096];
		std::string cgi_path = _route.cgi_path;
		if (realpath(cgi_path.c_str(), resolved_cgi) != NULL)
		{
			cgi_path = resolved_cgi;
		}

		std::string script_dir
			= script_path.substr(0, script_path.find_last_of('/'));
		chdir(script_dir.c_str());

		char *argv[] = {const_cast<char *>(cgi_path.c_str()),
						const_cast<char *>(script_path.c_str()),
						NULL};

		execve(cgi_path.c_str(), argv, &env_ptrs[0]);
		exit(1);
	}

	// Parent process
	close(stdin_pipe[0]);
	close(stdout_pipe[1]);

	// Write POST body to child stdin
	if (_request.get_method() == "POST")
	{
		const std::string &body = _request.get_body();
		if (!body.empty())
		{
			write(stdin_pipe[1], body.c_str(), body.size());
		}
	}
	close(stdin_pipe[1]); // signal EOF to child

	// Set stdout_pipe read end to non-blocking so poll() can time it out
	fcntl(stdout_pipe[0], F_SETFL, O_NONBLOCK);

	std::string output;
	char		buffer[4096];
	time_t		start	= time(NULL);
	const int	timeout = 5; // seconds

	while (true)
	{
		// Check wall-clock deadline
		if (difftime(time(NULL), start) >= timeout)
		{
			dprint("CGI: timeout reached, killing pid " << pid);
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			close(stdout_pipe[0]);
			return "";
		}

		struct pollfd pfd;
		pfd.fd		= stdout_pipe[0];
		pfd.events	= POLLIN;
		pfd.revents = 0;

		// Poll with 500ms slice so we re-check the deadline regularly
		int ret = poll(&pfd, 1, 500);

		if (ret < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			eprint("CGI: poll() error: " << strerror(errno));
			break;
		}

		if (ret == 0)
		{
			// poll timed out this slice — loop back to check deadline
			continue;
		}

		if (pfd.revents & POLLIN)
		{
			ssize_t bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer));
			if (bytes_read > 0)
			{
				output.append(buffer, bytes_read);
			}
			else if (bytes_read == 0)
			{
				// EOF — child closed its write end
				break;
			}
			else if (errno != EAGAIN && errno != EWOULDBLOCK)
			{
				eprint("CGI: read() error: " << strerror(errno));
				break;
			}
		}

		if (pfd.revents & (POLLHUP | POLLERR))
		{
			// Drain any remaining data after HUP
			ssize_t bytes_read;
			while ((bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer)))
				   > 0)
			{
				output.append(buffer, bytes_read);
			}
			break;
		}
	}

	close(stdout_pipe[0]);

	// Reap child (non-blocking; it should be done by now)
	int status = 0;
	waitpid(pid, &status, 0);

	dprint("CGI: child exited, output size = " << output.size());

	return output;
}
