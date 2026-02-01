#include "../inc/webserv.hpp"
#include "../inc/Server.hpp"

Server::Server(int port) {
	// create server socket
	_serverFd = socket(AF_INET, SOCK_STREAM, 0); //AF_INET (address family for IPv4),
												//SOCK_STREAM (socket type using TCP/IP)
	if (_serverFd < 0) {
		strerror(errno);
		return ;
	}

	// create struct with local address info
	_addr.sin_family = AF_INET; //address family
	_addr.sin_addr.s_addr = INADDR_ANY; //IP address (0.0.0.0)
	_addr.sin_port = htons(port); //port nbr in network byte order

	// choose address + port that server will use
	if (bind(_serverFd, (sockaddr*)&_addr, sizeof(_addr)) < 0) {
		strerror(errno);
		close(_serverFd);
		return ;
	}
	// get server ready for incoming connection attempts
	if (listen(_serverFd, 10) < 0) {
		strerror(errno);
		close(_serverFd);
		return ;
	}

	// init server poll
	pollfd serverPoll;
	serverPoll.fd = _serverFd;
	serverPoll.events = POLLIN;
	serverPoll.revents = 0;
	_pollFds.push_back(serverPoll);

	setNonBlocking(_serverFd);
}

Server::~Server() {
	for (size_t i = 0; i < _clients.size(); i++)
		close(_clients[i].fd);
	close(_serverFd);
}

void Server::start() {
	std::cout << "Server is listening... \n";
	// event loop
	while (1) {
		int ready;

		// setup poll to sleep until an event occurs
		ready = poll(&_pollFds[0], _pollFds.size(), -1);
		if (ready < 0) {
			strerror(errno);
			break;
		}
		// check which fd had an event, if is ready for reading
		for (size_t i = 0; i < _pollFds.size(); ) {
			// when client disconnects
			if (_pollFds[i].revents & (POLLHUP | POLLERR | POLLNVAL)) {
				if (_pollFds[i].fd != _serverFd) {
					removeClient(i);
					continue;
				}
			}

			// when client tries to connect
			if (_pollFds[i].revents & POLLIN) {
				if (_pollFds[i].fd == _serverFd)
					acceptClient();
				else {
					Client* client = getClient(_pollFds[i].fd);
					if (!client)
						continue;
					char buffer[4096];
					int bytes = read(client->fd, buffer, sizeof(buffer));
					if (bytes == 0) {
						removeClient(i);
						continue;
					}
					else if (bytes < 0) {
						if (errno != EAGAIN && errno != EWOULDBLOCK) {
							removeClient(i);
							continue;
						}
					}
					else {
						client->buffer.append(buffer, bytes);
						std::cout << "Buffer size for fd "
									<< client->fd << ": "
									<< client->buffer.size() << "\n";
					}
					if (client->hasCompleteHeader(client->buffer)) {
						std::cout << "Full HTTP headers recieved for fd "
									<< client->fd << "\n";
					}
				}
			}

			// clear handled event
			_pollFds[i].revents = 0;
			i++;
		}
	}
}

void Server::acceptClient() {
	// client info
	sockaddr_in client_addr;
	int client_fd;
	socklen_t client_len;

	// waits a connection, when it arrives, opens a new socket to communicate
	client_len = sizeof(client_addr);
	client_fd = accept(_serverFd, (sockaddr*)&client_addr, &client_len);
	if (client_fd < 0) {
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			strerror(errno);
		return ;
	}
	setNonBlocking(client_fd);
	_clients.push_back(Client(client_fd));

	// init client poll
	pollfd clientPoll;
	clientPoll.fd = client_fd;
	clientPoll.events = POLLIN;
	clientPoll.revents = 0;
	_pollFds.push_back(clientPoll);

	std::cout << "Client (fd=" << client_fd << ") connected to server!\n";
}

void Server::removeClient(size_t index) {
	int fd;
	std::vector<Client>::iterator it;

	fd = _pollFds[index].fd;
	std::cout << "Client (fd=" << fd << ") disconnected\n";
	close(fd);

	// remove from clients vector
	for (it = _clients.begin(); it != _clients.end(); it++) {
		if (it->fd == fd) {
			_clients.erase(it);
			break;
		}
	}

	// remove from poll vector
	_pollFds.erase(_pollFds.begin() + index);
}

void Server::setNonBlocking(int fd) {
	// get fd status flags
	int flags = fcntl(fd, F_GETFL);
	if (flags < 0) {
		strerror(errno);
		return ;
	}
	// change fd status flags
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
		strerror(errno);
	}
}

Client* Server::getClient(int fd) {
	for (size_t i = 0; i < _clients.size(); i++) {
		if (_clients[i].fd == fd)
			return &_clients[i];
	}
	return NULL;
}
