#pragma once

#include <string>

class Client {
private:
	int			_fd;
	std::string _buffer;
public:
	Client(int fd);
	~Client();

	int 				getFd() const;
	const std::string&	getBuffer() const;

	void	appendToBuffer(const char* data, size_t len);
	bool	hasCompleteHeader() const;
	void	clearBuffer();
};
