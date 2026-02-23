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
	int fd;
	int port;

	ServerSocket(int f, int p) : fd(f), port(p) {}
};

class Server
{
  private:
	std::vector<ServerSocket>	server_fds_;
	sockaddr_in			  		addr_;
	std::map<int, Client> 		clients_;
	std::vector<pollfd>	  		poll_fds_;
	std::vector<ServerConfig>	configs_;

	static void			  set_non_blocking(int fd);
	static void			  set_reuse_addr(int fd);
	void				  remove_client(size_t index);
	Client				 *get_client(int fd);
	const RouteConfig	 *match_route(const std::string &uri, const ServerConfig &config) const;
	bool				  handle_poll_errors(size_t &idx);
	bool				  handle_poll_input(size_t &idx);
	bool				  isListeningSocket(int fd) const;
	void				  handle_client_read(size_t &idx);
	bool				  handle_poll_output(size_t &idx);

	Server(const Server &);
	Server &operator=(const Server &);

  public:
	Server(const std::vector<ServerConfig> &configs);
	~Server();

	void start();
	void accept_client(size_t server_index);
};

#endif
