#ifndef CGI_HPP
# define CGI_HPP

# include <vector>
# include <string>
# include <unistd.h>

class CGI {
	private:
		std::vector<std::string> env_;

	public:
		CGI();
		CGI(const CGI& other);
		CGI& operator=(const CGI& other);
		~CGI();

		std::string	execute();
};

#endif