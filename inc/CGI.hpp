#ifndef CGI_HPP
#define CGI_HPP

#include <cerrno>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include "Config.hpp"
#include "HTTPRequest.hpp"
#include "utils_print.hpp"

class CGI
{
  private:
	std::vector<std::string> _env;

	void					 _setEnvironment();
	std::string				 _getScriptPath() const;

	// The HTTP request that triggered the CGI execution, used to set
	// environment variables and determine the script path
	const HTTPRequest		&_request;

	// The route configuration that matched the request, used to determine the
	// script path and other settings
	const RouteConfig		&_route;

  public:
	CGI(const HTTPRequest &request, const RouteConfig &route);
	CGI(const CGI &other);
	CGI &operator=(const CGI &other);
	~CGI();

	// Executes the CGI script and returns its raw stdout output
	// (CGI response headers + blank line + body).
	// Returns an empty string on execution failure or timeout.
	std::string execute();
};

#endif
