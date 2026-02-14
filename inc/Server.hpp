#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"

#include "../inc/HTTPRequest.hpp"
#include "../inc/signals.hpp"
#include "../inc/utils_print.hpp"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h> //sockaddr_in
#include <poll.h>
#include <sstream>
#include <unistd.h>
#include <vector>

class Server
{
  private:
	int					  serverFd_;
	sockaddr_in			  addr_;
	std::map<int, Client> clients_;
	std::vector<pollfd>	  pollFds_;

	static void			  set_non_blocking(int fd);
	void				  remove_client(size_t index);
	Client				 *get_client(int fd);

  public:
	Server(int port);
	Server(const Server &src);
	Server &operator=(const Server &src);
	~Server();

	void start();
	void accept_client();
};

#endif