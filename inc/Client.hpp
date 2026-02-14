#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
  private:
	int			fd_;
	std::string buffer_;

  public:
	Client(int fd);
	Client(const Client &src);
	Client &operator=(const Client &src);
	~Client();

	int				   get_fd() const;
	const std::string &get_buffer() const;

	void			   append_to_buffer(const char *data, size_t len);
	bool			   has_complete_header() const;
	void			   clear_buffer();
};

#endif
