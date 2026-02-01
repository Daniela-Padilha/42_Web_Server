#pragma once

#include <netinet/in.h> //sockaddr_in
#include <vector>
#include <map>
#include <fcntl.h>
#include <poll.h>
#include "Client.hpp"

class Server {
private:
	int						_serverFd;
	sockaddr_in				_addr;
	std::map<int, Client>	_clients;
	std::vector<pollfd>		_pollFds;

	void setNonBlocking(int fd);
	void removeClient(size_t index);
	Client* getClient(int fd);
public:
	Server(int port);
	~Server();
	void start();
	void acceptClient();
};