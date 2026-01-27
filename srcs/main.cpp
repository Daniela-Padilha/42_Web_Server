#include "../inc/webserv.hpp"
#include "../inc/Server.hpp"

int main(void) {
	Server server(8080);
	server.start();
	return 0;
}
