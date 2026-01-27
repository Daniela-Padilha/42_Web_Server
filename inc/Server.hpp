#pragma once

#include <netinet/in.h> //sockaddr_in
#include <vector>

class Server {
private:
	int			_serverFd;
	sockaddr_in	_addr;
	std::vector<int> _clients;

public:
	Server(int port);
	~Server();
	void start();
	void acceptClient();
};