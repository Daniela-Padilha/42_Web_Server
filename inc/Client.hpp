#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
  private:
	int			fd_;
	std::string buffer_;
	std::string response_;

  public:
	Client(int fd);
	Client(const Client &src);
	Client &operator=(const Client &src);
	~Client();

	int				   get_fd() const;
	const std::string &get_buffer() const;
	const std::string &get_response() const;

	void			   append_to_buffer(const char *data, size_t len);
	bool			   has_complete_header() const;
	void			   clear_buffer();

	void			   set_response(const std::string &response);
	void			   clear_response();
};

#endif
