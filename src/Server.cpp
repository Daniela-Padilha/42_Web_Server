#include "../inc/Server.hpp"
#include "../inc/webserver.hpp"

Server::Server(int port)
{
	// create server socket
	serverFd_ = socket(AF_INET,
					   SOCK_STREAM,
					   0); // AF_INET (address family for IPv4),
						   // SOCK_STREAM (socket type using TCP/IP)
	if (serverFd_ < 0)
	{
		strerror(errno);
		return;
	}

	// create struct with local address info
	addr_.sin_family	  = AF_INET;	 // address family
	addr_.sin_addr.s_addr = INADDR_ANY;	 // IP address (0.0.0.0)
	addr_.sin_port		  = htons(port); // port nbr in network byte order

	// choose address + port that server will use
	if (bind(serverFd_, (sockaddr *) &addr_, sizeof(addr_)) < 0)
	{
		strerror(errno);
		close(serverFd_);
		return;
	}
	// get server ready for incoming connection attempts
	if (listen(serverFd_, 10) < 0)
	{
		strerror(errno);
		close(serverFd_);
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

Server::~Server()
{
	std::map<int, Client>::iterator it;
	for (it = clients_.begin(); it != clients_.end(); it++)
		close(it->first);
	close(serverFd_);
}

void Server::start()
{
	std::cout << "Server is listening... \n";
	// event loop
	while (1)
	{
		int ready;

		// setup poll to sleep until an event occurs
		ready = poll(&pollFds_[0], pollFds_.size(), -1);
		if (ready < 0)
		{
			strerror(errno);
			break;
		}
		// check which fd had an event, if is ready for reading
		for (size_t i = 0; i < pollFds_.size();)
		{
			// when client disconnects
			if (pollFds_[i].revents & (POLLHUP | POLLERR | POLLNVAL))
			{
				if (pollFds_[i].fd != serverFd_)
				{
					remove_client(i);
					continue;
				}
			}

			// when client tries to connect
			if (pollFds_[i].revents & POLLIN)
			{
				if (pollFds_[i].fd == serverFd_)
				{
					accept_client();
				}
				else
				{
					Client *client = get_client(pollFds_[i].fd);
					if (!client)
						continue;
					char buffer[4096];
					int	 bytes = read(client->get_fd(), buffer, sizeof(buffer));
					if (bytes == 0)
					{
						remove_client(i);
						continue;
					}
					else if (bytes < 0)
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
						std::cout << "Buffer size for fd " << client->get_fd()
								  << ": " << client->get_buffer().size()
								  << "\n";
					}
					if (client->has_complete_header())
					{
						std::cout << "Full HTTP headers received for fd "
								  << client->get_fd() << "\n";
						// later: pass buffer to parser
						client->clear_buffer();
					}
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
			strerror(errno);
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
		strerror(errno);
		return;
	}
	// change fd status flags
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		strerror(errno);
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
