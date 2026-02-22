#ifndef CGI_HPP
# define CGI_HPP

# include <vector>
# include <string>
# include <unistd.h>
# include "HTTPRequest.hpp"

class CGI {
	private:
		std::vector<std::string> _env;
		void	_setEnvironment();

		const HTTPRequest& _request;

	public:
		CGI();
		CGI(const CGI& other);
		CGI& operator=(const CGI& other);
		~CGI();

		std::string	execute();
};

#endif