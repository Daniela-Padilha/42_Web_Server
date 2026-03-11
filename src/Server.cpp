#include "../inc/Server.hpp"

#include "../inc/HTTPHandler.hpp"
#include "../inc/HTTPResponse.hpp"

static const int	MAX_PENDING_CONNECTIONS = 10;
static const size_t READ_BUFFER_SIZE		= 4096;
static const int	POLL_TIMEOUT			= 1000;
static const double CLIENT_TIMEOUT			= 60.0;

Server::Server(const std::vector<ServerConfig> &configs) :
	configs_(configs),
	init_ok_(true)
{
	for (size_t i = 0; i < configs_.size(); i++)
	{
		int fd = socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0)
		{
			std::cerr << "Error: socket creation failed: " << strerror(errno)
					  << '\n';
			cleanup_sockets_();
			init_ok_ = false;
			return;
		}
		addr_.sin_family	  = AF_INET;
		addr_.sin_addr.s_addr = INADDR_ANY;
		addr_.sin_port		  = htons(configs_[i].port);

		set_reuse_addr(fd);

		// choose address + port that server will use
		if (bind(fd, reinterpret_cast<sockaddr *>(&addr_), sizeof(addr_)) < 0)
		{
			std::cerr << "Error: bind failed: " << strerror(errno) << '\n';
			close(fd);
			cleanup_sockets_();
			init_ok_ = false;
			return;
		}

		// get server ready for incoming connection attempts
		if (listen(fd, MAX_PENDING_CONNECTIONS) < 0)
		{
			std::cerr << "Error: listen failed: " << strerror(errno) << '\n';
			close(fd);
			cleanup_sockets_();
			init_ok_ = false;
			return;
		}

		set_non_blocking(fd);

		server_fds_.push_back(ServerSocket(fd, configs[i].port, i));

		// init server poll
		pollfd server_poll;
		server_poll.fd		= fd;
		server_poll.events	= POLLIN;
		server_poll.revents = 0;
		poll_fds_.push_back(server_poll);
	}
}

void Server::cleanup_sockets_()
{
	for (size_t i = 0; i < server_fds_.size(); i++)
	{
		if (server_fds_[i].fd != -1)
		{
			close(server_fds_[i].fd);
		}
	}
	server_fds_.clear();
	poll_fds_.clear();
}

Server::~Server()
{
	std::map<int, Client>::iterator it;
	for (it = clients_.begin(); it != clients_.end(); it++)
	{
		close(it->first);
	}
	for (size_t i = 0; i < server_fds_.size(); i++)
	{
		if (server_fds_[i].fd != -1)
		{
			close(server_fds_[i].fd);
		}
	}
}

void Server::start()
{
	if (!init_ok_)
	{
		return;
	}

	std::cout << "Server is listening... \n";
	// event loop
	while (g_looping != 0)
	{
		int ready = 0;

		// setup poll to sleep until an event occurs
		ready = poll(&poll_fds_[0], poll_fds_.size(), POLL_TIMEOUT);
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

		// check timeouts
		time_t now = time(NULL);
		for (size_t i = 0; i < poll_fds_.size();)
		{
			if (isListeningSocket(poll_fds_[i].fd))
			{
				i++;
				continue;
			}
			Client *clnt = get_client(poll_fds_[i].fd);
			if ((clnt != 0)
				&& difftime(now, clnt->get_last_activity()) > CLIENT_TIMEOUT)
			{
				std::cout << "Client (fd=" << clnt->get_fd() << ") timed out\n";
				remove_client(i);
				continue;
			}
			i++;
		}
	}
}

bool Server::handle_poll_errors(size_t &idx)
{
	if ((poll_fds_[idx].revents & (POLLHUP | POLLERR | POLLNVAL)) == 0)
	{
		return false;
	}
	if (!isListeningSocket(poll_fds_[idx].fd))
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
	if (isListeningSocket(poll_fds_[idx].fd))
	{
		accept_client(poll_fds_[idx].fd);
		return false;
	}
	handle_client_read(idx);
	return false;
}

bool Server::isListeningSocket(int fd) const
{
	for (size_t i = 0; i < server_fds_.size(); i++)
	{
		if (server_fds_[i].fd == fd)
			return true;
	}
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
	const ServerConfig &conf = configs_[client->get_server_index()];
	client->update_activity();

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
	req.set_max_body_size(conf.client_max_body_size);
	bool complete = req.parse(client->get_buffer());

	if (req.is_error())
	{
		HTTPResponse response;
		if (req.is_body_too_large())
		{
			response = HTTPResponse::error_413(
				HTTPHandler::get_error_page(413, conf));
		}
		else
		{
			response = HTTPResponse::error_400(
				HTTPHandler::get_error_page(400, conf));
		}
		client->set_response(response.to_string());
		poll_fds_[idx].events |= POLLOUT;
		client->clear_buffer();
	}
	else if (complete)
	{
		std::string uri	 = req.get_request_target();
		size_t		qpos = uri.find('?');
		if (qpos != std::string::npos)
		{
			uri = uri.substr(0, qpos);
		}
		const RouteConfig *route = match_route(uri, conf);

		// Directory trailing-slash redirect:
		// If the URI doesn't end with '/', check whether adding
		// a slash would match a more specific route. If so, the
		// client asked for a directory without the trailing slash
		// and we must redirect with 301.
		if (!uri.empty() && uri[uri.size() - 1] != '/')
		{
			const RouteConfig *slash_route = match_route(uri + "/", conf);
			if (slash_route != NULL
				&& (route == NULL
					|| slash_route->path.size() > route->path.size()))
			{
				HTTPResponse response = HTTPResponse::redirect_301(uri + "/");
				client->set_response(response.to_string());
				poll_fds_[idx].events |= POLLOUT;
				client->clear_buffer();
				return;
			}
		}

		HTTPResponse response = HTTPHandler::handle_request(req, route, conf);
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

	const std::string &response	 = client->get_response();
	size_t			   offset	 = client->get_response_offset();
	size_t			   total_len = response.length();
	size_t			   remaining = total_len - offset;

	ssize_t			   sent
		= send(client->get_fd(), response.c_str() + offset, remaining, 0);
	if (sent < 0)
	{
		eprint("Server: Error sending data to client");
		std::cerr << "Error sending data to client\n";
		remove_client(idx);
		return true;
	}
	client->advance_response_offset(sent);
	client->update_activity();
	dprint("Server: Sent " << sent << " bytes to client (total: "
						   << offset + sent << "/" << total_len << ")");

	if (client->get_response_offset() >= total_len)
	{
		client->clear_response();
		poll_fds_[idx].events &= ~POLLOUT;
		remove_client(idx);
		return true;
	}

	return false;
}

void Server::accept_client(int server_fd)
{
	// Find server index from fd
	size_t server_index = 0;
	for (size_t i = 0; i < server_fds_.size(); i++)
	{
		if (server_fds_[i].fd == server_fd)
		{
			server_index = server_fds_[i].index;
			break;
		}
	}

	// client info
	sockaddr_in client_addr;
	int			client_fd  = 0;
	socklen_t	client_len = 0;

	// waits a connection, when it arrives, opens a new socket to communicate
	client_len = sizeof(client_addr);
	client_fd  = accept(server_fds_[server_index].fd,
						reinterpret_cast<sockaddr *>(&client_addr),
						&client_len);
	if (client_fd < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			std::cerr << "accept: " << strerror(errno) << '\n';
		}
		return;
	}
	set_non_blocking(client_fd);
	clients_.insert(std::make_pair(client_fd, Client(client_fd, server_index)));

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
	poll_fds_.erase(poll_fds_.begin() + static_cast<std::ptrdiff_t>(index));
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

const RouteConfig *Server::match_route(const std::string  &uri,
									   const ServerConfig &config) const
{
	const RouteConfig *best_match  = NULL;
	size_t			   best_length = 0;

	for (size_t i = 0; i < config.routes.size(); i++)
	{
		const RouteConfig &route = config.routes[i];
		if (uri.compare(0, route.path.size(), route.path) == 0)
		{
			if (uri.size() == route.path.size() || uri[route.path.size()] == '/'
				|| route.path == "/"
				|| route.path[route.path.size() - 1] == '/')
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
