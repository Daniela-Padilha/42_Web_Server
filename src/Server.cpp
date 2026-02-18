#include "../inc/Server.hpp"

#include "../inc/HTTPHandler.hpp"
#include "../inc/HTTPResponse.hpp"

static const int	MAX_PENDING_CONNECTIONS = 10;
static const size_t READ_BUFFER_SIZE		= 4096;
static const int	POLL_INDEFINITE			= -1;

Server::Server(const ServerConfig &config) :
	server_fd_(socket(AF_INET, SOCK_STREAM, 0)),
	config_(config)
{
	// create server socket
	// AF_INET (address family for IPv4),
	// SOCK_STREAM (socket type using TCP/IP)
	if (server_fd_ < 0)
	{
		std::cerr << "Error: socket creation failed: " << strerror(errno)
				  << '\n';
		return;
	}

	// create struct with local address info
	addr_.sin_family	  = AF_INET;			// address family
	addr_.sin_addr.s_addr = INADDR_ANY;			// IP address (0.0.0.0)
	addr_.sin_port		  = htons(config.port); // port from config

	set_reuse_addr(server_fd_);

	// choose address + port that server will use
	if (bind(server_fd_, (sockaddr *) &addr_, sizeof(addr_)) < 0)
	{
		std::cerr << "Error: bind failed: " << strerror(errno) << '\n';
		close(server_fd_);
		server_fd_ = -1;
		return;
	}
	// get server ready for incoming connection attempts
	if (listen(server_fd_, MAX_PENDING_CONNECTIONS) < 0)
	{
		std::cerr << "Error: listen failed: " << strerror(errno) << '\n';
		close(server_fd_);
		server_fd_ = -1;
		return;
	}

	// init server poll
	pollfd server_poll;
	server_poll.fd		= server_fd_;
	server_poll.events	= POLLIN;
	server_poll.revents = 0;
	poll_fds_.push_back(server_poll);

	set_non_blocking(server_fd_);
}

Server::Server(const Server &src) :
	server_fd_(src.server_fd_),
	addr_(src.addr_),
	clients_(src.clients_),
	poll_fds_(src.poll_fds_),
	config_(src.config_)
{
}

Server &Server::operator=(const Server &src)
{
	if (this != &src)
	{
		this->server_fd_ = src.server_fd_;
		this->addr_		 = src.addr_;
		this->clients_	 = src.clients_;
		this->poll_fds_	 = src.poll_fds_;
		this->config_	 = src.config_;
	}
	return *this;
}

Server::~Server()
{
	std::map<int, Client>::iterator it;
	for (it = clients_.begin(); it != clients_.end(); it++)
	{
		close(it->first);
	}
	if (server_fd_ != -1)
	{
		close(server_fd_);
	}
}

void Server::start()
{
	if (server_fd_ == -1)
	{
		return;
	}

	std::cout << "Server is listening... \n";
	// event loop
	while (g_looping != 0)
	{
		int ready = 0;

		// setup poll to sleep until an event occurs
		ready = poll(&poll_fds_[0], poll_fds_.size(), POLL_INDEFINITE);
		if (ready < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			std::cerr << "poll: " << strerror(errno) << '\n';
			break;
		}
		// check which fd had an event
		for (size_t idx = 0; idx < poll_fds_.size();)
		{
			if (handle_poll_errors(idx))
			{
				continue;
			}
			if (handle_poll_input(idx))
			{
				continue;
			}
			if (handle_poll_output(idx))
			{
				continue;
			}
			// clear handled event
			poll_fds_[idx].revents = 0;
			idx++;
		}
	}
}

bool Server::handle_poll_errors(size_t &idx)
{
	if ((poll_fds_[idx].revents & (POLLHUP | POLLERR | POLLNVAL)) == 0)
	{
		return false;
	}
	if (poll_fds_[idx].fd != server_fd_)
	{
		remove_client(idx);
		return true;
	}
	return false;
}

bool Server::handle_poll_input(size_t &idx)
{
	if ((poll_fds_[idx].revents & POLLIN) == 0)
	{
		return false;
	}
	if (poll_fds_[idx].fd == server_fd_)
	{
		accept_client();
		return false;
	}
	handle_client_read(idx);
	return false;
}

void Server::handle_client_read(size_t &idx)
{
	Client *client = get_client(poll_fds_[idx].fd);
	if (client == 0)
	{
		idx++;
		return;
	}
	char	buffer[READ_BUFFER_SIZE];
	ssize_t bytes = read(client->get_fd(), buffer, sizeof(buffer));
	if (bytes == 0)
	{
		remove_client(idx);
		return;
	}
	if (bytes < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			remove_client(idx);
		}
		return;
	}
	client->append_to_buffer(buffer, bytes);
	dprint("Server: Buffer size for fd " << client->get_fd() << ": "
										 << client->get_buffer().size());

	if (!client->has_complete_header())
	{
		return;
	}
	dprint("Server: Full HTTP headers received for fd " << client->get_fd());

	HTTPRequest req;
	bool		complete = req.parse(client->get_buffer());

	if (req.is_error())
	{
		HTTPResponse response
			= HTTPResponse::error_400(HTTPHandler::get_error_page(400));
		client->set_response(response.to_string());
		poll_fds_[idx].events |= POLLOUT;
		client->clear_buffer();
	}
	else if (complete)
	{
		const RouteConfig *route = match_route(req.get_request_target());
		HTTPResponse	   response
			= HTTPHandler::handle_request(req, route, config_);
		client->set_response(response.to_string());
		poll_fds_[idx].events |= POLLOUT;
		client->clear_buffer();
	}
	// else: body not fully received yet — keep buffer,
	// wait for more data on next poll() cycle
}

bool Server::handle_poll_output(size_t &idx)
{
	if ((poll_fds_[idx].revents & POLLOUT) == 0)
	{
		return false;
	}
	Client *client = get_client(poll_fds_[idx].fd);
	if (client == 0 || client->get_response().empty())
	{
		return false;
	}
	ssize_t sent = send(client->get_fd(),
						client->get_response().c_str(),
						client->get_response().length(),
						0);
	if (sent < 0)
	{
		eprint("Server: Error sending data to client");
	}
	else
	{
		dprint("Server: Sent " << sent << " bytes to client");
	}
	client->clear_response();
	poll_fds_[idx].events &= ~POLLOUT;

	// For now, close connection after sending response
	remove_client(idx);
	return true;
}

void Server::accept_client()
{
	// client info
	sockaddr_in client_addr;
	int			client_fd  = 0;
	socklen_t	client_len = 0;

	// waits a connection, when it arrives, opens a new socket to communicate
	client_len = sizeof(client_addr);
	client_fd  = accept(server_fd_, (sockaddr *) &client_addr, &client_len);
	if (client_fd < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			std::cerr << "accept: " << strerror(errno) << '\n';
		}
		return;
	}
	set_non_blocking(client_fd);
	clients_.insert(std::make_pair(client_fd, Client(client_fd)));

	// init client poll
	pollfd client_poll;
	client_poll.fd		= client_fd;
	client_poll.events	= POLLIN;
	client_poll.revents = 0;
	poll_fds_.push_back(client_poll);

	std::cout << "Client (fd=" << client_fd << ") connected to server!\n";
}

void Server::remove_client(size_t index)
{
	int fd = 0;

	fd = poll_fds_[index].fd;
	std::cout << "Client (fd=" << fd << ") disconnected\n";
	close(fd);

	// remove from clients map
	clients_.erase(fd);

	// remove from poll vector
	poll_fds_.erase(poll_fds_.begin() + index);
}

void Server::set_non_blocking(int fd)
{
	// get fd status flags
	int flags = fcntl(fd, F_GETFL);
	if (flags < 0)
	{
		std::cerr << "fcntl F_GETFL: " << strerror(errno) << '\n';
		return;
	}
	// change fd status flags
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		std::cerr << "fcntl F_SETFL: " << strerror(errno) << '\n';
	}
}

// allow immediate reuse of address/port after server shutdown
void Server::set_reuse_addr(int fd)
{
	int opt = 1;
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		std::cerr << "Error: setsockopt SO_REUSEADDR failed: "
				  << strerror(errno) << '\n';
	}
}

Client *Server::get_client(int fd)
{
	std::map<int, Client>::iterator it = clients_.find(fd);
	if (it == clients_.end())
	{
		return NULL;
	}
	return &it->second;
}

const RouteConfig *Server::match_route(const std::string &uri) const
{
	const RouteConfig *best_match  = NULL;
	size_t			   best_length = 0;

	for (size_t i = 0; i < config_.routes.size(); i++)
	{
		const RouteConfig &route = config_.routes[i];
		if (uri.compare(0, route.path.size(), route.path) == 0)
		{
			if (uri.size() == route.path.size() || uri[route.path.size()] == '/'
				|| route.path == "/")
			{
				if (route.path.size() > best_length)
				{
					best_length = route.path.size();
					best_match	= &route;
				}
			}
		}
	}

	return best_match;
}
