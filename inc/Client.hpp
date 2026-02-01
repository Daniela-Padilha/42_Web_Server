#pragma once

#include <string>

class Client {
public:
	int			fd;
	std::string buffer;

	Client(int fd_);
	~Client();
};
