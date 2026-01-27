#include "../inc/webserv.hpp"

int socket_creator(void) {
	int socket_fd;

	// create socket
	socket_fd = socket(AF_INET, SOCK_STREAM, 0); //AF_INET (address family for IPv4),
												//SOCK_STREAM (socket type using TCP/IP)
	if (socket_fd < 0) {
		strerror(errno);
		return 1;
	}

	// create struct with local address info
	sockaddr_in addr;
	addr.sin_family = AF_INET; //address family
	addr.sin_port = htons(8080); //port nbr in network byte order
	addr.sin_addr.s_addr = INADDR_ANY; //IP address (0.0.0.0)

	// choose address + port that socket will use
	if (bind(socket_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
		strerror(errno);
		close(socket_fd);
		return 1;
	}

	// get socket ready for incoming connection attempts
	if (listen(socket_fd, 10) < 0) {
		strerror(errno);
		close(socket_fd);
		return 1;
	}

	std::cout << "Server is listening on port 8080...\n";

	// create client
	sockaddr_in client_addr;
	int client_fd;
	socklen_t client_len;

	// waits a connection, when it arrives, opens a new socket to communicate
	client_len = sizeof(client_addr);
	client_fd = accept(socket_fd, (sockaddr*)&client_addr, &client_len);
	if (client_fd < 0) {
		strerror(errno);
		close(socket_fd);
		return 1;
	}

	std::cout << "Client connected to server!\n";

	// cleanup
	close(client_fd);
	close(socket_fd);
	return 0;
}
