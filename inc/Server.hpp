#pragma once

#include <netinet/in.h> //sockaddr_in
#include <vector>
#include <fcntl.h>

class Server {
private:
	int			_serverFd;
	sockaddr_in	_addr;
	std::vector<int> _clients;

	void setNonBlocking(int fd);
public:
	Server(int port);
	~Server();
	void start();
	void acceptClient();
};