#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <csignal>
#include <ctime>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "HTTPRequest.hpp"

class Client
{
  private:
	int			fd_;
	std::string buffer_;
	std::string response_;
	time_t		last_activity_;
	size_t		response_offset_;
	size_t		server_index_;

	HTTPRequest request_;
	bool		request_started_;

	// CGI streaming state
	pid_t		cgi_pid_;
	int			cgi_stdin_fd_;	// write end: send body to CGI
	int			cgi_stdout_fd_; // read end:  collect CGI output
	std::string
		 cgi_output_; // accumulator for CGI stdout (headers + partial body)
	bool cgi_active_;
	std::string
		cgi_stdin_pending_; // body bytes buffered waiting for stdin pipe space
	size_t cgi_stdin_pending_offset_; // how many bytes already consumed
	bool   cgi_stdin_eof_; // all body bytes have been sent to CGI stdin
	size_t cgi_body_bytes_forwarded_; // bytes forwarded to CGI stdin so far
	size_t cgi_body_content_length_;  // expected body length (0 = unknown)
	bool   cgi_headers_sent_; // true once HTTP response headers sent to client

	// CGI chunked decoding state
	bool   cgi_chunked_;

	enum ChunkedDecState
	{
		CHUNK_DEC_SIZE_,
		CHUNK_DEC_DATA_,
		CHUNK_DEC_DATA_CRLF_,
		CHUNK_DEC_TRAILERS_,
		CHUNK_DEC_DONE_
	};

	ChunkedDecState cgi_chunk_state_;
	size_t			cgi_chunk_remaining_;
	std::string		cgi_chunk_buf_;

	std::string
		   cgi_send_pending_; // CGI body bytes waiting to be sent to client
	size_t cgi_send_pending_offset_; // how many bytes already sent

  public:
	Client(int fd, size_t server_index);
	Client(const Client &src);
	Client &operator=(const Client &src);
	~Client();

	int				   get_fd() const;
	const std::string &get_buffer() const;
	const std::string &get_response() const;
	time_t			   get_last_activity() const;
	size_t			   get_response_offset() const;
	size_t			   get_server_index() const;

	HTTPRequest		  &get_request();
	bool			   is_request_started() const;
	void			   set_request_started(bool started);

	void			   update_activity();
	void			   advance_response_offset(size_t bytes);
	void			   reset_response_offset();

	void			   append_to_buffer(const char *data, size_t len);
	bool			   has_complete_header() const;
	void			   clear_buffer();
	void			   clear_raw_buffer();

	void			   set_response(const std::string &response);
	void			   clear_response();

	// CGI streaming helpers
	void			   start_cgi(pid_t pid, int stdin_fd, int stdout_fd);
	bool			   is_cgi_active() const;
	int				   get_cgi_stdout_fd() const;
	void			   reset_cgi_stdout_fd();
	int				   get_cgi_stdin_fd() const;
	pid_t			   get_cgi_pid() const;
	void			   append_cgi_output(const char *data, size_t len);
	const std::string &get_cgi_output() const;
	void			   close_cgi_stdin();
	void			   finish_cgi();

	// CGI stdin buffering helpers
	void			   append_cgi_stdin_pending(const char *data, size_t len);
	const std::string &get_cgi_stdin_pending() const;
	const char		  *get_cgi_stdin_pending_ptr() const;
	size_t			   get_cgi_stdin_pending_size() const;
	void			   consume_cgi_stdin_pending(size_t bytes);
	bool			   is_cgi_stdin_pending_empty() const;
	bool			   is_cgi_stdin_eof() const;
	void			   set_cgi_stdin_eof(bool val);
	size_t			   get_cgi_body_bytes_forwarded() const;
	void			   add_cgi_body_bytes_forwarded(size_t n);
	size_t			   get_cgi_body_content_length() const;
	void			   set_cgi_body_content_length(size_t len);

	// CGI chunked decoding helpers
	void			   set_cgi_chunked(bool val);
	bool			   is_cgi_chunked() const;
	void			   init_cgi_chunk_decoder(int state, size_t remaining);
	bool			   decode_chunked_for_cgi(const char  *data,
											  size_t	   len,
											  std::string &decoded,
											  bool		  &eof);

	// CGI streaming to client
	bool			   is_cgi_headers_sent() const;
	void			   set_cgi_headers_sent(bool val);
	void			   append_cgi_send_pending(const char *data, size_t len);
	const std::string &get_cgi_send_pending() const;
	const char		  *get_cgi_send_pending_ptr() const;
	size_t			   get_cgi_send_pending_size() const;
	void			   consume_cgi_send_pending(size_t bytes);
};

#endif
