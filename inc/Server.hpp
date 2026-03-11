#ifndef SERVER_HPP
#define SERVER_HPP

#include "Client.hpp"
#include "Config.hpp"

#include "../inc/HTTPRequest.hpp"
#include "../inc/signals.hpp"
#include "../inc/utils_print.hpp"

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h> //sockaddr_in
#include <poll.h>
#include <sstream>
#include <unistd.h>
#include <vector>

struct ServerSocket
{
	int	   fd;
	int	   port;
	size_t index;

	ServerSocket(int f, int p, size_t i) : fd(f), port(p), index(i)
	{
	}
};

class Server
{
  private:
	std::vector<ServerSocket> server_fds_;
	sockaddr_in				  addr_;
	std::map<int, Client>	  clients_;
	std::vector<pollfd>		  poll_fds_;
	std::vector<ServerConfig> configs_;
	bool					  init_ok_;

	static void				  set_non_blocking(int fd);
	static void				  set_reuse_addr(int fd);
	void					  cleanup_sockets_();
	void					  remove_client(size_t index);
	Client					 *get_client(int fd);
	const RouteConfig		 *match_route(const std::string	 &uri,
										  const ServerConfig &config) const;
	bool					  handle_poll_errors(size_t &idx);
	bool					  handle_poll_input(size_t &idx);
	bool					  isListeningSocket(int fd) const;
	void					  handle_client_read(size_t &idx);
	bool					  handle_poll_output(size_t &idx);

	Server(const Server &);
	Server &operator=(const Server &);

  public:
	Server(const std::vector<ServerConfig> &configs);
	~Server();

	void start();
	void accept_client(int server_fd);

	bool is_ok() const
	{
		return init_ok_;
	}
};

#endif
