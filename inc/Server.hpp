#pragma once

#include <netinet/in.h> //sockaddr_in
#include <vector>
#include <fcntl.h>
#include <poll.h>

class Server {
private:
	int					_serverFd;
	sockaddr_in			_addr;
	std::vector<int>	_clients;
	std::vector<pollfd>	_pollFds;

	void setNonBlocking(int fd);
	void removeClient(size_t index);
public:
	Server(int port);
	~Server();
	void start();
	void acceptClient();
};