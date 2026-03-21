#include "../inc/Server.hpp"

#include "../inc/CGI.hpp"
#include "../inc/HTTPHandler.hpp"
#include "../inc/HTTPResponse.hpp"

#include <sys/socket.h>

#ifndef MSG_NOSIGNAL
# define MSG_NOSIGNAL 0
#endif

static const int	MAX_PENDING_CONNECTIONS = 128;
static const size_t READ_BUFFER_SIZE		= 65536;
static const int	POLL_TIMEOUT			= 100;
static const double CLIENT_TIMEOUT			= 120.0;
static const size_t CGI_BACKPRESSURE_LIMIT	= 4194304; // 4 MB
// Max bytes to move per fd per poll iteration to ensure fair scheduling
// across many concurrent connections.  Each fd gets to read/write at most
// this many bytes before yielding to the next fd.
static const size_t FAIR_IO_LIMIT = 262144; // 256 KB

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
#ifdef STATUS_DUMP
	time_t last_status_dump_ = time(NULL);
#endif
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

#ifdef STATUS_DUMP
		// Periodic status dump for debugging concurrent CGI issues
		{
			time_t now_dbg = time(NULL);
			if (difftime(now_dbg, last_status_dump_) >= 3.0
				&& (!cgi_fd_to_client_fd_.empty() || clients_.size() > 5))
			{
				last_status_dump_ = now_dbg;
				std::cerr << "=== STATUS: " << clients_.size() << " clients, "
						  << cgi_fd_to_client_fd_.size() << " cgi_out, "
						  << cgi_stdin_fd_to_client_fd_.size() << " cgi_in, "
						  << poll_fds_.size() << " poll_fds, ready=" << ready
						  << " ===\n";
				for (std::map<int, Client>::iterator ci = clients_.begin();
					 ci != clients_.end();
					 ++ci)
				{
					Client &c	  = ci->second;
					short	evts  = 0;
					short	revts = 0;
					for (size_t pi = 0; pi < poll_fds_.size(); ++pi)
						if (poll_fds_[pi].fd == c.get_fd())
						{
							evts  = poll_fds_[pi].events;
							revts = poll_fds_[pi].revents;
							break;
						}
					if (c.is_cgi_active() || c.is_cgi_headers_sent())
					{
						// Also find CGI stdout and stdin events
						short cgi_out_ev = 0, cgi_out_rev = 0;
						short cgi_in_ev = 0, cgi_in_rev = 0;
						int	  cgi_out_fd = c.get_cgi_stdout_fd();
						int	  cgi_in_fd	 = c.get_cgi_stdin_fd();
						for (size_t pi = 0; pi < poll_fds_.size(); ++pi)
						{
							if (cgi_out_fd != -1
								&& poll_fds_[pi].fd == cgi_out_fd)
							{
								cgi_out_ev	= poll_fds_[pi].events;
								cgi_out_rev = poll_fds_[pi].revents;
							}
							if (cgi_in_fd != -1
								&& poll_fds_[pi].fd == cgi_in_fd)
							{
								cgi_in_ev  = poll_fds_[pi].events;
								cgi_in_rev = poll_fds_[pi].revents;
							}
						}
						std::cerr
							<< "  fd=" << c.get_fd()
							<< " active=" << c.is_cgi_active()
							<< " sin_pend=" << c.get_cgi_stdin_pending_size()
							<< " snd_pend=" << c.get_cgi_send_pending_size()
							<< " sin_eof=" << c.is_cgi_stdin_eof()
							<< " hdr=" << c.is_cgi_headers_sent()
							<< " ev=" << evts << " rev=" << revts
							<< " | out_fd=" << cgi_out_fd
							<< " out_ev=" << cgi_out_ev << "/" << cgi_out_rev
							<< " in_fd=" << cgi_in_fd << " in_ev=" << cgi_in_ev
							<< "/" << cgi_in_rev << "\n";
					}
				}
			}
		}
#endif
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

bool Server::isListeningSocket(int fd) const
{
	for (size_t i = 0; i < server_fds_.size(); i++)
	{
		if (server_fds_[i].fd == fd)
			return true;
	}
	return false;
}

bool Server::isCgiFd(int fd) const
{
	return cgi_fd_to_client_fd_.count(fd) > 0;
}

bool Server::isCgiStdinFd(int fd) const
{
	return cgi_stdin_fd_to_client_fd_.count(fd) > 0;
}

Client *Server::get_client_by_cgi_fd(int cgi_fd)
{
	std::map<int, int>::iterator it = cgi_fd_to_client_fd_.find(cgi_fd);
	if (it == cgi_fd_to_client_fd_.end())
		return NULL;
	return get_client(it->second);
}

bool Server::handle_poll_errors(size_t &idx)
{
	if ((poll_fds_[idx].revents & (POLLHUP | POLLERR | POLLNVAL)) == 0)
	{
		return false;
	}
	int fd = poll_fds_[idx].fd;
	if (!isListeningSocket(fd))
	{
		if (isCgiFd(fd))
		{
			// CGI stdout hung up — treat as if output is complete,
			// fall through to handle_poll_input which will read EOF
			return false;
		}
		if (isCgiStdinFd(fd))
		{
			// CGI stdin pipe broke (child died?) — remove it from poll
			Client *client = NULL;
			{
				std::map<int, int>::iterator it
					= cgi_stdin_fd_to_client_fd_.find(fd);
				if (it != cgi_stdin_fd_to_client_fd_.end())
					client = get_client(it->second);
			}
			cgi_stdin_fd_to_client_fd_.erase(fd);
			close(fd);
			if (client)
				client->close_cgi_stdin();
			poll_fds_.erase(poll_fds_.begin()
							+ static_cast<std::ptrdiff_t>(idx));
			return true;
		}
		remove_client(idx);
		return true;
	}
	return false;
}

bool Server::handle_poll_input(size_t &idx)
{
	if ((poll_fds_[idx].revents & POLLIN) == 0)
	{
		// Check for POLLHUP on CGI stdout fds even without POLLIN
		if ((poll_fds_[idx].revents & POLLHUP) && isCgiFd(poll_fds_[idx].fd))
		{
			handle_cgi_output(idx);
			return true;
		}
		return false;
	}
	int fd = poll_fds_[idx].fd;
	if (isListeningSocket(fd))
	{
		accept_client(fd);
		return false;
	}
	// CGI stdin fds are write-only — never readable; skip
	if (isCgiStdinFd(fd))
	{
		idx++;
		return true;
	}
	if (isCgiFd(fd))
	{
		handle_cgi_output(idx);
		return true;
	}
	handle_client_read(idx);
	return false;
}

void Server::handle_cgi_output(size_t &idx)
{
	int		cgi_fd = poll_fds_[idx].fd;
	Client *client = get_client_by_cgi_fd(cgi_fd);
	dprint("handle_cgi_output: cgi_fd=" << cgi_fd << " client="
										<< (client ? client->get_fd() : -1));

	if (client == NULL)
	{
		// Orphan CGI fd — just remove it from poll
		close(cgi_fd);
		cgi_fd_to_client_fd_.erase(cgi_fd);
		poll_fds_.erase(poll_fds_.begin() + static_cast<std::ptrdiff_t>(idx));
		return;
	}

	const ServerConfig &conf = configs_[client->get_server_index()];

	// Read from CGI stdout, bounded by FAIR_IO_LIMIT to ensure fair
	// scheduling across many concurrent connections.
	char				buf[READ_BUFFER_SIZE];
	size_t				total_read = 0;
	while (total_read < FAIR_IO_LIMIT)
	{
		ssize_t bytes = read(cgi_fd, buf, sizeof(buf));

		if (bytes > 0)
		{
			total_read += static_cast<size_t>(bytes);
			dprint("handle_cgi_output: read "
				   << bytes << " bytes from cgi_fd=" << cgi_fd);
			if (!client->is_cgi_headers_sent())
			{
				// Still collecting CGI headers
				client->append_cgi_output(buf, static_cast<size_t>(bytes));
				const std::string &raw	   = client->get_cgi_output();
				size_t			   hdr_end = raw.find("\r\n\r\n");
				if (hdr_end != std::string::npos)
				{
					// Parse CGI headers and build HTTP response header
					std::string	 cgi_header_part = raw.substr(0, hdr_end);
					HTTPResponse resp
						= HTTPHandler::parse_cgi_headers(cgi_header_part, conf);
					resp.set_header("Connection", "close");
					// Build only the header portion to send
					std::string http_header = resp.header_to_string();
					client->append_cgi_send_pending(http_header.c_str(),
													http_header.size());
					// Body bytes already received
					size_t body_start = hdr_end + 4;
					if (body_start < raw.size())
					{
						client->append_cgi_send_pending(
							raw.c_str() + body_start, raw.size() - body_start);
					}
					client->set_cgi_headers_sent(true);
					// Enable POLLOUT on client fd
					for (size_t i = 0; i < poll_fds_.size(); i++)
					{
						if (poll_fds_[i].fd == client->get_fd())
						{
							poll_fds_[i].events |= POLLOUT;
							break;
						}
					}
				}
			}
			else
			{
				// Stream body bytes directly
				client->append_cgi_send_pending(buf,
												static_cast<size_t>(bytes));
				// Re-enable POLLOUT if needed
				for (size_t i = 0; i < poll_fds_.size(); i++)
				{
					if (poll_fds_[i].fd == client->get_fd())
					{
						poll_fds_[i].events |= POLLOUT;
						break;
					}
				}
			}
			// Back-pressure: if the send buffer is large, stop reading from
			// CGI and let handle_poll_output drain it to the client first.
			// This prevents unbounded memory growth for large CGI responses.
			if (client->get_cgi_send_pending_size() > CGI_BACKPRESSURE_LIMIT)
			{
				// Temporarily disable POLLIN on CGI fd
				poll_fds_[idx].events &= ~POLLIN;
				idx++;
				return;
			}
			continue; // try to read more (within limit)
		}

		if (bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
		{
			// No more data right now — come back next poll event
			idx++;
			return;
		}

		// bytes == 0: EOF; or bytes < 0 with a real error — finalize
		int	   client_fd  = client->get_fd();
		size_t client_idx = 0;
		for (size_t i = 0; i < poll_fds_.size(); i++)
		{
			if (poll_fds_[i].fd == client_fd)
			{
				client_idx = i;
				break;
			}
		}
		// Remove CGI stdout fd from poll
		cgi_fd_to_client_fd_.erase(cgi_fd);
		close(cgi_fd);
		client->reset_cgi_stdout_fd();
		poll_fds_.erase(poll_fds_.begin() + static_cast<std::ptrdiff_t>(idx));
		// Adjust client_idx if it was shifted by the erase
		if (idx < client_idx)
			client_idx--;
		// Don't increment idx — we just erased this slot
		finalize_cgi(*client, client_idx);
		return;
	}
	// Hit FAIR_IO_LIMIT — yield to let other fds make progress
	idx++;
}

void Server::finalize_cgi(Client &client, size_t idx)
{
	dprint("finalize_cgi: fd=" << client.get_fd() << " headers_sent="
							   << client.is_cgi_headers_sent());
	const ServerConfig &conf = configs_[client.get_server_index()];

	if (!client.is_cgi_headers_sent())
	{
		// CGI exited without sending headers — try to parse whatever we got
		const std::string &raw = client.get_cgi_output();
		HTTPResponse	   response;
		if (raw.empty())
		{
			response = HTTPResponse::error_500(
				HTTPHandler::get_error_page(500, conf));
		}
		else
		{
			response = HTTPHandler::parse_cgi_response(raw, conf);
		}
		response.set_header("Connection", "close");
		client.finish_cgi();
		client.set_response(response.to_string());
		if (idx < poll_fds_.size() && poll_fds_[idx].fd == client.get_fd())
		{
			poll_fds_[idx].events |= POLLOUT;
		}
		else
		{
			for (size_t i = 0; i < poll_fds_.size(); i++)
			{
				if (poll_fds_[i].fd == client.get_fd())
				{
					poll_fds_[i].events |= POLLOUT;
					break;
				}
			}
		}
	}
	else
	{
		// Headers already sent streaming; CGI stdout is now EOF.
		// Reap the child and mark cgi as done.
		client.finish_cgi();
		// The pending send buffer will be drained by handle_poll_output.
		// Make sure POLLOUT is set so we finish draining.
		for (size_t i = 0; i < poll_fds_.size(); i++)
		{
			if (poll_fds_[i].fd == client.get_fd())
			{
				poll_fds_[i].events |= POLLOUT;
				break;
			}
		}
	}
	client.clear_buffer();
}

// Called when poll signals POLLOUT on the CGI stdin fd — flush buffered body
// bytes into the pipe.
void Server::handle_cgi_stdin_writable(size_t &idx)
{
	int							 stdin_fd = poll_fds_[idx].fd;

	std::map<int, int>::iterator it = cgi_stdin_fd_to_client_fd_.find(stdin_fd);
	if (it == cgi_stdin_fd_to_client_fd_.end())
	{
		// Orphan — remove
		close(stdin_fd);
		poll_fds_.erase(poll_fds_.begin() + static_cast<std::ptrdiff_t>(idx));
		return;
	}

	Client *client = get_client(it->second);
	if (client == NULL)
	{
		cgi_stdin_fd_to_client_fd_.erase(it);
		close(stdin_fd);
		poll_fds_.erase(poll_fds_.begin() + static_cast<std::ptrdiff_t>(idx));
		return;
	}

	const std::string &pending = client->get_cgi_stdin_pending();
	(void) pending;
	if (!client->is_cgi_stdin_pending_empty())
	{
		// Bounded loop to flush pending data to CGI stdin pipe.
		size_t total_written = 0;
		while (!client->is_cgi_stdin_pending_empty()
			   && total_written < FAIR_IO_LIMIT)
		{
			ssize_t written = write(stdin_fd,
									client->get_cgi_stdin_pending_ptr(),
									client->get_cgi_stdin_pending_size());
			if (written > 0)
			{
				total_written += static_cast<size_t>(written);
				client->consume_cgi_stdin_pending(static_cast<size_t>(written));
			}
			else
				break; // EAGAIN or error — stop for now
		}
	}

	// If all pending data flushed and client has signalled EOF, close stdin
	if (client->is_cgi_stdin_pending_empty() && client->is_cgi_stdin_eof())
	{
		cgi_stdin_fd_to_client_fd_.erase(it);
		client->close_cgi_stdin();
		// Remove from poll
		poll_fds_.erase(poll_fds_.begin() + static_cast<std::ptrdiff_t>(idx));
		return;
	}

	// Re-enable POLLIN on client fd if stdin buffer has drained below the
	// back-pressure limit (matching the disable in handle_client_read).
	if (client->get_cgi_stdin_pending_size() < CGI_BACKPRESSURE_LIMIT)
	{
		int client_fd = client->get_fd();
		for (size_t i = 0; i < poll_fds_.size(); i++)
		{
			if (poll_fds_[i].fd == client_fd)
			{
				poll_fds_[i].events |= POLLIN;
				break;
			}
		}
	}

	// If no more pending data, stop listening for POLLOUT (avoid busy-loop)
	if (client->is_cgi_stdin_pending_empty())
	{
		poll_fds_[idx].events &= ~POLLOUT;
	}
	idx++;
}

// Fork the CGI child, register its stdout fd in poll, and begin writing
// body data we already have.  Called as soon as request headers are complete
// and the route is a CGI route.
void Server::dispatch_cgi(Client &client, size_t idx)
{
	dprint("dispatch_cgi: ENTERED for fd=" << client.get_fd());
	const ServerConfig &conf = configs_[client.get_server_index()];
	const HTTPRequest  &req	 = client.get_request();
	std::string			uri	 = req.get_request_target();
	size_t				qpos = uri.find('?');
	if (qpos != std::string::npos)
		uri = uri.substr(0, qpos);
	const RouteConfig *route = match_route(uri, conf);
	if (route == NULL)
		return;

	// Build env and prepare pipes
	int stdin_pipe[2];
	int stdout_pipe[2];
	if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0)
	{
		eprint("Server: dispatch_cgi: pipe() failed");
		HTTPResponse err
			= HTTPResponse::error_500(HTTPHandler::get_error_page(500, conf));
		err.set_header("Connection", "close");
		client.set_response(err.to_string());
		poll_fds_[idx].events |= POLLOUT;
		client.clear_buffer();
		return;
	}

	CGI						 cgi(req, *route);
	// Build environment (re-use CGI class internals via a temporary execute
	// approach, but we need to fork ourselves here for async operation)
	// We duplicate CGI's fork logic inline so we can keep the pipes
	// non-blocking
	std::vector<std::string> env_vec;
	{
		// Minimal env — we replicate _setEnvironment logic here
		std::string request_target = req.get_request_target();
		std::string script_name	   = request_target;
		std::string query_string   = "";
		size_t		qp			   = request_target.find('?');
		if (qp != std::string::npos)
		{
			script_name	 = request_target.substr(0, qp);
			query_string = request_target.substr(qp + 1);
		}
		env_vec.push_back("GATEWAY_INTERFACE=CGI/1.1");
		env_vec.push_back("SERVER_PROTOCOL=" + req.get_protocol());
		env_vec.push_back("SERVER_NAME=" + req.get_header_value("host"));
		env_vec.push_back("REQUEST_METHOD=" + req.get_method());
		env_vec.push_back("SCRIPT_NAME=" + script_name);
		env_vec.push_back("PATH_INFO=" + request_target);
		env_vec.push_back("REQUEST_URI=" + request_target);
		env_vec.push_back("QUERY_STRING=" + query_string);
		env_vec.push_back("CONTENT_TYPE="
						  + req.get_header_value("content-type"));
		env_vec.push_back("CONTENT_LENGTH="
						  + req.get_header_value("content-length"));

		// Forward all HTTP headers as HTTP_* env vars (RFC 3875 4.1.18)
		const std::map<std::string, std::string> &hdrs = req.get_headers();
		for (std::map<std::string, std::string>::const_iterator hit
			 = hdrs.begin();
			 hit != hdrs.end();
			 ++hit)
		{
			// Skip headers already handled as their own CGI vars
			if (hit->first == "content-type" || hit->first == "content-length")
				continue;
			// Convert header name: lowercase to uppercase, '-' to '_'
			std::string env_name = "HTTP_";
			for (size_t ci = 0; ci < hit->first.size(); ++ci)
			{
				char c = hit->first[ci];
				if (c == '-')
					env_name += '_';
				else
					env_name += static_cast<char>(
						std::toupper(static_cast<unsigned char>(c)));
			}
			env_vec.push_back(env_name + "=" + hit->second);
		}

		// Get script path (route.root + path after route prefix)
		std::string rt	= request_target;
		size_t		qp2 = rt.find('?');
		if (qp2 != std::string::npos)
			rt = rt.substr(0, qp2);
		const std::string &prefix = route->path;
		if (!prefix.empty() && prefix != "/"
			&& rt.compare(0, prefix.size(), prefix) == 0)
		{
			rt = rt.substr(prefix.size());
			if (rt.empty() || rt[0] != '/')
				rt = "/" + rt;
		}
		std::string script_path = route->root + rt;

		env_vec.push_back("SCRIPT_FILENAME=" + script_path);
		env_vec.push_back("REDIRECT_STATUS=200");
	}

	// Resolve cgi_path and script_path to absolute
	std::string rt2 = req.get_request_target();
	size_t		qp3 = rt2.find('?');
	if (qp3 != std::string::npos)
		rt2 = rt2.substr(0, qp3);
	const std::string &pfx = route->path;
	if (!pfx.empty() && pfx != "/" && rt2.compare(0, pfx.size(), pfx) == 0)
	{
		rt2 = rt2.substr(pfx.size());
		if (rt2.empty() || rt2[0] != '/')
			rt2 = "/" + rt2;
	}
	std::string script_path = route->root + rt2;

	char		resolved_script[4096];
	if (realpath(script_path.c_str(), resolved_script) != NULL)
		script_path = resolved_script;

	std::string cgi_path = route->cgi_path;
	char		resolved_cgi[4096];
	if (realpath(cgi_path.c_str(), resolved_cgi) != NULL)
		cgi_path = resolved_cgi;

	std::string script_dir
		= script_path.substr(0, script_path.find_last_of('/'));

	pid_t pid = fork();
	if (pid < 0)
	{
		eprint("Server: dispatch_cgi: fork() failed");
		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);
		HTTPResponse err
			= HTTPResponse::error_500(HTTPHandler::get_error_page(500, conf));
		err.set_header("Connection", "close");
		client.set_response(err.to_string());
		poll_fds_[idx].events |= POLLOUT;
		client.clear_buffer();
		return;
	}

	if (pid == 0)
	{
		// Child
		dup2(stdin_pipe[0], STDIN_FILENO);
		dup2(stdout_pipe[1], STDOUT_FILENO);
		close(stdin_pipe[0]);
		close(stdin_pipe[1]);
		close(stdout_pipe[0]);
		close(stdout_pipe[1]);

		// Close all inherited fds (server sockets, client connections, etc.)
		// to prevent the child from holding references to them.
		long max_fd = sysconf(_SC_OPEN_MAX);
		if (max_fd < 0)
			max_fd = 1024;
		for (int fd = STDERR_FILENO + 1; fd < max_fd; ++fd)
			close(fd);

		std::vector<char *> env_ptrs;
		for (size_t i = 0; i < env_vec.size(); ++i)
			env_ptrs.push_back(const_cast<char *>(env_vec[i].c_str()));
		env_ptrs.push_back(NULL);

		chdir(script_dir.c_str());

		char *argv[] = {const_cast<char *>(cgi_path.c_str()),
						const_cast<char *>(script_path.c_str()),
						NULL};
		execve(cgi_path.c_str(), argv, &env_ptrs[0]);
		exit(1);
	}

	// Parent
	close(stdin_pipe[0]);
	close(stdout_pipe[1]);

	// Make both pipe ends non-blocking
	{
		int fl = fcntl(stdout_pipe[0], F_GETFL, 0);
		if (fl >= 0)
			fcntl(stdout_pipe[0], F_SETFL, fl | O_NONBLOCK);
	}
	{
		int fl = fcntl(stdin_pipe[1], F_GETFL, 0);
		if (fl >= 0)
			fcntl(stdin_pipe[1], F_SETFL, fl | O_NONBLOCK);
	}

	// Increase pipe buffer sizes to 1MB to reduce round-trips for large bodies
	{
		static const int PIPE_SZ = 1048576;
#ifdef F_SETPIPE_SZ
		fcntl(stdin_pipe[1], F_SETPIPE_SZ, PIPE_SZ);
		fcntl(stdout_pipe[0], F_SETPIPE_SZ, PIPE_SZ);
#endif
	}

	// Determine transfer encoding and content-length
	std::string cl_str = req.get_header_value("content-length");
	size_t		cl
		= cl_str.empty() ? 0 : static_cast<size_t>(std::atol(cl_str.c_str()));
	bool te_chunked = false;
	{
		std::string te = req.get_header_value("transfer-encoding");
		for (size_t i = 0; i < te.size(); ++i)
			te[i] = static_cast<char>(
				std::tolower(static_cast<unsigned char>(te[i])));
		te_chunked = (te.find("chunked") != std::string::npos);
	}

	// Collect decoded body bytes to write to CGI stdin
	std::string body_to_write = req.get_body();
	bool		body_complete = (!te_chunked && cl == 0)
						 || (!te_chunked && body_to_write.size() >= cl);

	dprint("dispatch_cgi: cl=" << cl << " te_chunked=" << te_chunked
							   << " body_complete=" << body_complete
							   << " body_size=" << body_to_write.size());

	dprint("dispatch_cgi: fork returned pid="
		   << pid << " stdin_w=" << stdin_pipe[1]
		   << " stdout_r=" << stdout_pipe[0]);
	client.start_cgi(pid, stdin_pipe[1], stdout_pipe[0]);

	// Store content-length for EOF detection in handle_client_read
	client.set_cgi_body_content_length(cl);

	// For chunked requests, enable the chunked decoder and seed it with
	// the correct state from HTTPRequest's partial parse.
	if (te_chunked)
	{
		client.set_cgi_chunked(true);
		// Initialize decoder state to match where HTTPRequest left off.
		// HTTPRequest::get_chunked_state() maps to:
		//   0=CHUNK_SIZE_, 1=CHUNK_DATA_, 2=CHUNK_DATA_CRLF_, 3=TRAILERS_
		int	   rs = req.get_chunked_state();
		size_t cr = req.get_chunk_remaining();
		client.init_cgi_chunk_decoder(rs, cr);

		// Feed any leftover data in HTTPRequest's buffer through decoder
		const std::string &leftover = req.get_internal_buffer();
		if (!leftover.empty())
		{
			std::string decoded;
			bool		chunk_eof = false;
			client.decode_chunked_for_cgi(leftover.c_str(),
										  leftover.size(),
										  decoded,
										  chunk_eof);
			if (!decoded.empty())
				body_to_write += decoded;
			if (chunk_eof)
				body_complete = true;
		}
	}

	// Write decoded body bytes to CGI stdin (non-blocking)
	size_t written_so_far = 0;
	if (!body_to_write.empty())
	{
		ssize_t w
			= write(stdin_pipe[1], body_to_write.c_str(), body_to_write.size());
		if (w > 0)
			written_so_far = static_cast<size_t>(w);
	}
	client.add_cgi_body_bytes_forwarded(body_to_write.size());

	// If the initial write didn't send everything, buffer the remainder
	if (!body_to_write.empty() && written_so_far < body_to_write.size())
	{
		client.append_cgi_stdin_pending(body_to_write.c_str() + written_so_far,
										body_to_write.size() - written_so_far);
	}

	if (body_complete)
	{
		if (client.is_cgi_stdin_pending_empty())
		{
			// Nothing left to write — close stdin immediately.
			// start_cgi already stored stdin_pipe[1]; close_cgi_stdin
			// closes the fd and resets it to -1.
			client.close_cgi_stdin();
		}
		else
		{
			// Mark EOF so once pending is flushed we close stdin
			client.set_cgi_stdin_eof(true);
		}
	}

	// Register CGI stdout fd in poll (POLLIN)
	cgi_fd_to_client_fd_[stdout_pipe[0]] = client.get_fd();
	{
		pollfd cgi_poll;
		cgi_poll.fd		 = stdout_pipe[0];
		cgi_poll.events	 = POLLIN;
		cgi_poll.revents = 0;
		poll_fds_.push_back(cgi_poll);
	}

	// Register CGI stdin fd in poll (POLLOUT) if there's pending data or
	// body is not yet complete (more data will come from client)
	int stored_stdin_fd = client.get_cgi_stdin_fd();
	if (stored_stdin_fd != -1
		&& (!body_complete || !client.is_cgi_stdin_pending_empty()))
	{
		cgi_stdin_fd_to_client_fd_[stored_stdin_fd] = client.get_fd();
		pollfd stdin_poll;
		stdin_poll.fd	   = stored_stdin_fd;
		stdin_poll.events  = POLLOUT;
		stdin_poll.revents = 0;
		poll_fds_.push_back(stdin_poll);
	}
}

void Server::handle_client_read(size_t &idx)
{
	Client *client = get_client(poll_fds_[idx].fd);

	if (client == NULL)
	{
		idx++;
		return;
	}

	// If CGI is already active for this client, we just need to stream
	// incoming body bytes to the CGI's stdin pipe.
	// Bounded by FAIR_IO_LIMIT to ensure fair scheduling.
	if (client->is_cgi_active())
	{
		char   buf[READ_BUFFER_SIZE];
		size_t total_read = 0;
		while (total_read < FAIR_IO_LIMIT)
		{
			// Back-pressure: if too much data is already buffered for
			// CGI stdin, stop reading from the client to avoid unbounded
			// memory growth.  The poll loop will re-enable POLLIN when
			// handle_cgi_stdin_writable drains the buffer.
			if (client->get_cgi_stdin_pending_size() > CGI_BACKPRESSURE_LIMIT)
			{
				poll_fds_[idx].events &= ~POLLIN;
				return;
			}
			ssize_t bytes = read(client->get_fd(), buf, sizeof(buf));
			if (bytes == 0)
			{
				// Client closed connection — mark EOF
				client->set_cgi_stdin_eof(true);
				if (client->is_cgi_stdin_pending_empty())
					client->close_cgi_stdin();
				return;
			}
			if (bytes < 0)
			{
				if (errno != EAGAIN && errno != EWOULDBLOCK)
				{
					client->set_cgi_stdin_eof(true);
					if (client->is_cgi_stdin_pending_empty())
						client->close_cgi_stdin();
				}
				// EAGAIN — no more data available right now
				return;
			}
			total_read += static_cast<size_t>(bytes);
			client->update_activity();

			int stdin_fd = client->get_cgi_stdin_fd();
			if (stdin_fd == -1)
				return;

			// Determine the data to forward to CGI stdin.
			// For chunked requests, decode the chunk framing first.
			const char *fwd_data = buf;
			size_t		fwd_len	 = static_cast<size_t>(bytes);
			std::string decoded;
			bool		chunk_eof = false;

			if (client->is_cgi_chunked())
			{
				bool ok = client->decode_chunked_for_cgi(
					buf, static_cast<size_t>(bytes), decoded, chunk_eof);
				if (!ok)
				{
					// Chunked decode error — close stdin to let CGI finish
					client->set_cgi_stdin_eof(true);
					if (client->is_cgi_stdin_pending_empty())
						client->close_cgi_stdin();
					return;
				}
				fwd_data = decoded.c_str();
				fwd_len	 = decoded.size();
			}

			// Track bytes forwarded for EOF detection (raw bytes for
			// Content-Length mode, decoded bytes for chunked mode)
			if (!client->is_cgi_chunked())
				client->add_cgi_body_bytes_forwarded(
					static_cast<size_t>(bytes));

			// Write decoded/raw body to CGI stdin (non-blocking)
			if (fwd_len > 0)
			{
				ssize_t written = write(stdin_fd, fwd_data, fwd_len);
				size_t	sent = (written > 0) ? static_cast<size_t>(written) : 0;

				// Buffer any bytes that didn't fit in the pipe
				if (sent < fwd_len)
				{
					client->append_cgi_stdin_pending(fwd_data + sent,
													 fwd_len - sent);
					// Ensure stdin fd is being monitored for POLLOUT
					if (cgi_stdin_fd_to_client_fd_.find(stdin_fd)
						== cgi_stdin_fd_to_client_fd_.end())
					{
						cgi_stdin_fd_to_client_fd_[stdin_fd] = client->get_fd();
						pollfd sp;
						sp.fd	   = stdin_fd;
						sp.events  = POLLOUT;
						sp.revents = 0;
						poll_fds_.push_back(sp);
					}
					else
					{
						for (size_t i = 0; i < poll_fds_.size(); i++)
						{
							if (poll_fds_[i].fd == stdin_fd)
							{
								poll_fds_[i].events |= POLLOUT;
								break;
							}
						}
					}
				}
			}

			// For chunked requests, check if the final chunk was received
			if (client->is_cgi_chunked() && chunk_eof)
			{
				if (client->is_cgi_stdin_pending_empty())
				{
					client->close_cgi_stdin();
					cgi_stdin_fd_to_client_fd_.erase(stdin_fd);
					for (size_t i = 0; i < poll_fds_.size(); i++)
					{
						if (poll_fds_[i].fd == stdin_fd)
						{
							poll_fds_.erase(poll_fds_.begin()
											+ static_cast<std::ptrdiff_t>(i));
							break;
						}
					}
				}
				else
				{
					client->set_cgi_stdin_eof(true);
				}
				return;
			}

			// For non-chunked: check Content-Length EOF
			if (!client->is_cgi_chunked())
			{
				size_t cl_expected = client->get_cgi_body_content_length();
				if (cl_expected > 0
					&& client->get_cgi_body_bytes_forwarded() >= cl_expected)
				{
					if (client->is_cgi_stdin_pending_empty())
					{
						client->close_cgi_stdin();
						cgi_stdin_fd_to_client_fd_.erase(stdin_fd);
						for (size_t i = 0; i < poll_fds_.size(); i++)
						{
							if (poll_fds_[i].fd == stdin_fd)
							{
								poll_fds_.erase(
									poll_fds_.begin()
									+ static_cast<std::ptrdiff_t>(i));
								break;
							}
						}
					}
					else
					{
						client->set_cgi_stdin_eof(true);
					}
					return;
				}
			}
		}
		// Hit FAIR_IO_LIMIT — yield to let other fds make progress
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

	// Feed only the newly-read bytes into the incremental request parser.
	HTTPRequest &req = client->get_request();
	std::string	 new_data(buffer, bytes);

	// On first data for this request, accumulate until headers are complete,
	// then determine the effective max_body_size from the matching route.
	if (!client->is_request_started())
	{
		client->append_to_buffer(buffer, bytes);
		if (!client->has_complete_header())
			return;

		// Headers are now complete — determine limit from the matching route.
		const std::string &buf = client->get_buffer();
		size_t			   sp1 = buf.find(' ');
		size_t sp2 = (sp1 != std::string::npos) ? buf.find(' ', sp1 + 1) :
												  std::string::npos;
		size_t effective_max_body = conf.client_max_body_size;
		if (sp1 != std::string::npos && sp2 != std::string::npos)
		{
			std::string peek_uri = buf.substr(sp1 + 1, sp2 - sp1 - 1);
			size_t		qp		 = peek_uri.find('?');
			if (qp != std::string::npos)
				peek_uri = peek_uri.substr(0, qp);
			const RouteConfig *peek_route = match_route(peek_uri, conf);
			if (peek_route != NULL && peek_route->has_client_max_body_size)
				effective_max_body = peek_route->client_max_body_size;
		}
		req.set_max_body_size(effective_max_body);
		client->set_request_started(true);

		new_data = client->get_buffer();
		client->clear_raw_buffer();
	}

	bool complete = req.parse(new_data);
	if (client->is_request_started())
		client->clear_raw_buffer();

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
		response.set_header("Connection", "close");
		client->set_response(response.to_string());
		// Stop reading from client — remaining body data must not be
		// parsed as a new request.  Only enable output for the response.
		poll_fds_[idx].events = POLLOUT;
		client->clear_buffer();
		return;
	}

	// Check if headers are done and the route is CGI — if so, dispatch CGI
	// immediately without waiting for the full body.
	dprint("CGI gate: headers_done=" << req.is_headers_done() << " cgi_active="
									 << client->is_cgi_active()
									 << " complete=" << complete);
	if (req.is_headers_done() && !client->is_cgi_active())
	{
		std::string peek_uri = req.get_request_target();
		size_t		qp		 = peek_uri.find('?');
		if (qp != std::string::npos)
			peek_uri = peek_uri.substr(0, qp);
		const RouteConfig *route = match_route(peek_uri, conf);
		dprint("CGI check: uri="
			   << peek_uri << " route=" << (route ? route->path : "NULL")
			   << " cgi_ext=" << (route ? route->cgi_extension : ""));
		if (route != NULL && !route->cgi_extension.empty())
		{
			// Check CGI extension matches
			size_t ext_len = route->cgi_extension.size();
			bool   matches = peek_uri.size() >= ext_len
						   && peek_uri.compare(peek_uri.size() - ext_len,
											   ext_len,
											   route->cgi_extension)
								  == 0;
			if (matches)
			{
				// Check method is allowed
				bool method_ok = route->allowed_methods.empty();
				for (size_t i = 0; i < route->allowed_methods.size(); i++)
				{
					if (route->allowed_methods[i] == req.get_method())
					{
						method_ok = true;
						break;
					}
				}
				if (method_ok)
				{
					dprint("DISPATCHING CGI for fd=" << client->get_fd());
					dispatch_cgi(*client, idx);
					return;
				}
			}
		}
	}

	if (!complete)
		return;

	// Full request received — handle normally
	std::string uri	 = req.get_request_target();
	size_t		qpos = uri.find('?');
	if (qpos != std::string::npos)
		uri = uri.substr(0, qpos);
	const RouteConfig *route = match_route(uri, conf);

	// Directory trailing-slash redirect
	if (!uri.empty() && uri[uri.size() - 1] != '/')
	{
		const RouteConfig *slash_route = match_route(uri + "/", conf);
		if (slash_route != NULL
			&& (route == NULL || slash_route->path.size() > route->path.size()))
		{
			HTTPResponse response = HTTPResponse::redirect_301(uri + "/");
			response.set_header("Connection", "close");
			client->set_response(response.to_string());
			poll_fds_[idx].events |= POLLOUT;
			client->clear_buffer();
			return;
		}
	}

	HTTPResponse response = HTTPHandler::handle_request(req, route, conf);
	response.set_header("Connection", "close");
	client->set_response(response.to_string());
	poll_fds_[idx].events |= POLLOUT;
	client->clear_buffer();
}

bool Server::handle_poll_output(size_t &idx)
{
	if ((poll_fds_[idx].revents & POLLOUT) == 0)
	{
		return false;
	}
	int fd = poll_fds_[idx].fd;

	// CGI stdin pipe is writable — flush pending body data
	if (isCgiStdinFd(fd))
	{
		handle_cgi_stdin_writable(idx);
		return true;
	}

	Client *client = get_client(fd);
	if (client == 0)
	{
		return false;
	}

	// CGI streaming mode: send from cgi_send_pending_
	if (client->is_cgi_active() || client->get_cgi_send_pending_size() > 0
		|| client->is_cgi_headers_sent())
	{
		dprint("handle_poll_output CGI: fd="
			   << fd << " cgi_active=" << client->is_cgi_active()
			   << " pending=" << client->get_cgi_send_pending_size()
			   << " headers_sent=" << client->is_cgi_headers_sent());
		// Bounded loop to flush buffered CGI data to client.
		size_t total_sent = 0;
		while (total_sent < FAIR_IO_LIMIT)
		{
			size_t psize = client->get_cgi_send_pending_size();
			if (psize == 0)
			{
				if (!client->is_cgi_active())
				{
					// CGI done and all data sent — disconnect
					client->clear_response();
					poll_fds_[idx].events &= ~POLLOUT;
					remove_client(idx);
					return true;
				}
				// CGI still running, no data buffered — disable POLLOUT
				poll_fds_[idx].events &= ~POLLOUT;
				return false;
			}
			ssize_t sent = send(fd,
								client->get_cgi_send_pending_ptr(),
								psize,
								MSG_NOSIGNAL);
			if (sent < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					// Socket buffer full — stop for now
					return false;
				}
				eprint("Server: Error sending CGI data to client");
				remove_client(idx);
				return true;
			}
			if (sent > 0)
			{
				total_sent += static_cast<size_t>(sent);
				client->consume_cgi_send_pending(static_cast<size_t>(sent));
				client->update_activity();
				// Re-enable POLLIN on CGI stdout fd if back-pressure
				// was applied and the buffer has drained enough.
				if (client->is_cgi_active()
					&& client->get_cgi_send_pending_size()
						   < CGI_BACKPRESSURE_LIMIT)
				{
					int cgi_stdout = client->get_cgi_stdout_fd();
					if (cgi_stdout != -1)
					{
						for (size_t i = 0; i < poll_fds_.size(); i++)
						{
							if (poll_fds_[i].fd == cgi_stdout)
							{
								poll_fds_[i].events |= POLLIN;
								break;
							}
						}
					}
				}
			}
		}
		// Hit FAIR_IO_LIMIT — yield to let other fds make progress
		return false;
	}

	if (client->get_response().empty())
	{
		return false;
	}

	const std::string &response	 = client->get_response();
	size_t			   offset	 = client->get_response_offset();
	size_t			   total_len = response.length();
	size_t			   remaining = total_len - offset;

	ssize_t			   sent = send(client->get_fd(),
						   response.c_str() + offset,
						   remaining,
						   MSG_NOSIGNAL);
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

	// Accept all pending connections in a loop to drain the backlog.
	// This is critical under high concurrency — a single POLLIN event
	// may correspond to many queued connections.
	while (true)
	{
		sockaddr_in client_addr;
		socklen_t	client_len = sizeof(client_addr);
		int			client_fd  = accept(server_fds_[server_index].fd,
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
		clients_.insert(
			std::make_pair(client_fd, Client(client_fd, server_index)));

		// init client poll
		pollfd client_poll;
		client_poll.fd		= client_fd;
		client_poll.events	= POLLIN;
		client_poll.revents = 0;
		poll_fds_.push_back(client_poll);

		dprint("Client (fd=" << client_fd << ") connected to server!");
	}
}

void Server::remove_client(size_t index)
{
	int fd = poll_fds_[index].fd;
	dprint("Client (fd=" << fd << ") disconnected");

	// Clean up any associated CGI fds before removing the client.
	// This prevents orphan fds and zombie processes when clients
	// disconnect during active CGI.
	std::map<int, Client>::iterator cit = clients_.find(fd);
	if (cit != clients_.end())
	{
		Client &cl = cit->second;

		// Clean up CGI stdout fd from poll and map
		int		cgi_out = cl.get_cgi_stdout_fd();
		if (cgi_out != -1)
		{
			cgi_fd_to_client_fd_.erase(cgi_out);
			for (size_t i = 0; i < poll_fds_.size(); i++)
			{
				if (poll_fds_[i].fd == cgi_out)
				{
					poll_fds_.erase(poll_fds_.begin()
									+ static_cast<std::ptrdiff_t>(i));
					if (i < index)
						index--;
					break;
				}
			}
		}

		// Clean up CGI stdin fd from poll and map
		int cgi_in = cl.get_cgi_stdin_fd();
		if (cgi_in != -1)
		{
			cgi_stdin_fd_to_client_fd_.erase(cgi_in);
			for (size_t i = 0; i < poll_fds_.size(); i++)
			{
				if (poll_fds_[i].fd == cgi_in)
				{
					poll_fds_.erase(poll_fds_.begin()
									+ static_cast<std::ptrdiff_t>(i));
					if (i < index)
						index--;
					break;
				}
			}
		}

		// Reap child and close pipe fds
		cl.finish_cgi();
	}

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
