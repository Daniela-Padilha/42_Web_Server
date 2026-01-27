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

	// binds socket to local address, to reserve port for incoming connections
	if (bind(socket_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
		strerror(errno);
		close(socket_fd);
		return 1;
	}

	// the socket is set to listen for incoming connection attempts
	if (listen(socket_fd, 10) < 0) {
		strerror(errno);
		close(socket_fd);
		return 1;
	}

	std::cout << "Server is listening on port 8080...\n";

	return 0;
}
