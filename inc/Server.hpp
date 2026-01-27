#pragma once

#include <netinet/in.h> //sockaddr_in

class Server {
private:
	int			_serverFd;
	sockaddr_in	_addr;
public:
	Server(int port);
	~Server();
	void start();
	void acceptClient();
};