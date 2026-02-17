#include "../inc/Server.hpp"

#include "../inc/HTTPHandler.hpp"
#include "../inc/HTTPResponse.hpp"

Server::Server(int port) : serverFd_(socket(AF_INET, SOCK_STREAM, 0))
{
	// create server socket
	// AF_INET (address family for IPv4),
	// SOCK_STREAM (socket type using TCP/IP)
	if (serverFd_ < 0)
	{
		std::cerr << "Error: socket creation failed: " << strerror(errno)
				  << '\n';
		return;
	}

	// create struct with local address info
	addr_.sin_family	  = AF_INET;	 // address family
	addr_.sin_addr.s_addr = INADDR_ANY;	 // IP address (0.0.0.0)
	addr_.sin_port		  = htons(port); // port nbr in network byte order

	set_reuse_addr(serverFd_);

	// choose address + port that server will use
	if (bind(serverFd_, (sockaddr *) &addr_, sizeof(addr_)) < 0)
	{
		std::cerr << "Error: bind failed: " << strerror(errno) << '\n';
		close(serverFd_);
		serverFd_ = -1;
		return;
	}
	// get server ready for incoming connection attempts
	if (listen(serverFd_, 10) < 0)
	{
		std::cerr << "Error: listen failed: " << strerror(errno) << '\n';
		close(serverFd_);
		serverFd_ = -1;
		return;
	}

	// init server poll
	pollfd serverPoll;
	serverPoll.fd	   = serverFd_;
	serverPoll.events  = POLLIN;
	serverPoll.revents = 0;
	pollFds_.push_back(serverPoll);

	set_non_blocking(serverFd_);
}

Server::Server(const Server &src) :
	serverFd_(src.serverFd_),
	addr_(src.addr_),
	clients_(src.clients_),
	pollFds_(src.pollFds_)
{
}

Server &Server::operator=(const Server &src)
{
	if (this != &src)
	{
		this->serverFd_ = src.serverFd_;
		this->addr_		= src.addr_;
		this->clients_	= src.clients_;
		this->pollFds_	= src.pollFds_;
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
	if (serverFd_ != -1)
	{
		close(serverFd_);
	}
}

void Server::start()
{
	if (serverFd_ == -1)
	{
		return;
	}

	std::cout << "Server is listening... \n";
	// event loop
	while (g_looping != 0)
	{
		int ready = 0;

		// setup poll to sleep until an event occurs
		ready = poll(&pollFds_[0], pollFds_.size(), -1);
		if (ready < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			std::cerr << "poll: " << strerror(errno) << '\n';
			break;
		}
		// check which fd had an event, if is ready for reading
		for (size_t i = 0; i < pollFds_.size();)
		{
			// when client disconnects
			if ((pollFds_[i].revents & (POLLHUP | POLLERR | POLLNVAL)) != 0)
			{
				if (pollFds_[i].fd != serverFd_)
				{
					remove_client(i);
					continue;
				}
			}

			// when client tries to connect
			if ((pollFds_[i].revents & POLLIN) != 0)
			{
				if (pollFds_[i].fd == serverFd_)
				{
					accept_client();
				}
				else
				{
					Client *client = get_client(pollFds_[i].fd);
					if (client == 0)
					{
						i++;
						continue;
					}
					char buffer[4096];
					int	 bytes = read(client->get_fd(), buffer, sizeof(buffer));
					if (bytes == 0)
					{
						remove_client(i);
						continue;
					}
					if (bytes < 0)
					{
						if (errno != EAGAIN && errno != EWOULDBLOCK)
						{
							remove_client(i);
							continue;
						}
					}
					else
					{
						client->append_to_buffer(buffer, bytes);
						dprint("Server: Buffer size for fd "
							   << client->get_fd() << ": "
							   << client->get_buffer().size());
					}

					if (client->has_complete_header())
					{
						dprint("Server: Full HTTP headers received for fd "
							   << client->get_fd());

						HTTPRequest req;
						bool		complete = req.parse(client->get_buffer());

						if (req.is_error())
						{
							HTTPResponse response = HTTPResponse::error_400();
							client->set_response(response.to_string());
							pollFds_[i].events |= POLLOUT;
							client->clear_buffer();
						}
						else if (complete)
						{
							HTTPHandler	 handler;
							HTTPResponse response
								= HTTPHandler::handle_request(req);
							client->set_response(response.to_string());
							pollFds_[i].events |= POLLOUT;
							client->clear_buffer();
						}
						// else: body not fully received yet — keep buffer,
						// wait for more data on next poll() cycle
					}
				}
			}

			// when client is ready to write
			if ((pollFds_[i].revents & POLLOUT) != 0)
			{
				Client *client = get_client(pollFds_[i].fd);
				if ((client != 0) && !client->get_response().empty())
				{
					int sent = send(client->get_fd(),
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
					pollFds_[i].events &= ~POLLOUT;

					// For now, close connection after sending response
					remove_client(i);
					continue;
				}
			}

			// clear handled event
			pollFds_[i].revents = 0;
			i++;
		}
	}
}

void Server::accept_client()
{
	// client info
	sockaddr_in client_addr;
	int			client_fd_ = 0;
	socklen_t	client_len = 0;

	// waits a connection, when it arrives, opens a new socket to communicate
	client_len = sizeof(client_addr);
	client_fd_ = accept(serverFd_, (sockaddr *) &client_addr, &client_len);
	if (client_fd_ < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			std::cerr << "accept: " << strerror(errno) << '\n';
		}
		return;
	}
	set_non_blocking(client_fd_);
	clients_.insert(std::make_pair(client_fd_, Client(client_fd_)));

	// init client poll
	pollfd clientPoll;
	clientPoll.fd	   = client_fd_;
	clientPoll.events  = POLLIN;
	clientPoll.revents = 0;
	pollFds_.push_back(clientPoll);

	std::cout << "Client (fd=" << client_fd_ << ") connected to server!\n";
}

void Server::remove_client(size_t index)
{
	int fd = 0;

	fd = pollFds_[index].fd;
	std::cout << "Client (fd=" << fd << ") disconnected\n";
	close(fd);

	// remove from clients map
	clients_.erase(fd);

	// remove from poll vector
	pollFds_.erase(pollFds_.begin() + index);
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
