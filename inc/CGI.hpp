#ifndef CGI_HPP
#define CGI_HPP

# include <vector>
# include <string>
# include <unistd.h>
# include <sys/wait.h>
# include <signal.h>

# include "HTTPRequest.hpp"
# include "Config.hpp"

class CGI {
	private:
		std::vector<std::string> _env;
		
		void	_setEnvironment();
		std::string _getScriptPath() const;
		void	_handleTimeout(pid_t pid);

		// The HTTP request that triggered the CGI execution, used to set environment variables and determine the script path
		const HTTPRequest& _request;
		
		// The route configuration that matched the request, used to determine the script path and other settings
		const RouteConfig& _route;

	public:
		CGI(const HTTPRequest& request, const RouteConfig& route);
		CGI(const CGI& other);
		CGI& operator=(const CGI& other);
		~CGI();

		std::string	execute();
};

#endif