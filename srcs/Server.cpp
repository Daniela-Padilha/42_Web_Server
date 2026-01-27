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
}

Server::~Server() {
	close(_serverFd);
}

void Server::start() {
	std::cout << "Server is listening... \n";
	acceptClient();
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
		strerror(errno);
		return ;
	}

	std::cout << "Client (fd=" << client_fd << ") connected to server!\n";
	close(client_fd);
}
