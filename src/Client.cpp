#include "../inc/Client.hpp"

Client::Client(int fd, size_t server_index) :
	fd_(fd),
	last_activity_(time(NULL)),
	response_offset_(0),
	server_index_(server_index),
	request_started_(false),
	cgi_pid_(-1),
	cgi_stdin_fd_(-1),
	cgi_stdout_fd_(-1),
	cgi_active_(false),
	cgi_stdin_pending_offset_(0),
	cgi_stdin_eof_(false),
	cgi_body_bytes_forwarded_(0),
	cgi_body_content_length_(0),
	cgi_headers_sent_(false),
	cgi_chunked_(false),
	cgi_chunk_state_(CHUNK_DEC_SIZE_),
	cgi_chunk_remaining_(0),
	cgi_send_pending_offset_(0)
{
}

Client::Client(const Client &src) :
	fd_(src.fd_),
	buffer_(src.buffer_),
	response_(src.response_),
	last_activity_(src.last_activity_),
	response_offset_(src.response_offset_),
	server_index_(src.server_index_),
	request_(src.request_),
	request_started_(src.request_started_),
	cgi_pid_(src.cgi_pid_),
	cgi_stdin_fd_(src.cgi_stdin_fd_),
	cgi_stdout_fd_(src.cgi_stdout_fd_),
	cgi_output_(src.cgi_output_),
	cgi_active_(src.cgi_active_),
	cgi_stdin_pending_(src.cgi_stdin_pending_),
	cgi_stdin_pending_offset_(src.cgi_stdin_pending_offset_),
	cgi_stdin_eof_(src.cgi_stdin_eof_),
	cgi_body_bytes_forwarded_(src.cgi_body_bytes_forwarded_),
	cgi_body_content_length_(src.cgi_body_content_length_),
	cgi_headers_sent_(src.cgi_headers_sent_),
	cgi_chunked_(src.cgi_chunked_),
	cgi_chunk_state_(src.cgi_chunk_state_),
	cgi_chunk_remaining_(src.cgi_chunk_remaining_),
	cgi_chunk_buf_(src.cgi_chunk_buf_),
	cgi_send_pending_(src.cgi_send_pending_),
	cgi_send_pending_offset_(src.cgi_send_pending_offset_)
{
}

Client &Client::operator=(const Client &src)
{
	if (this != &src)
	{
		this->fd_						= src.fd_;
		this->buffer_					= src.buffer_;
		this->response_					= src.response_;
		this->last_activity_			= src.last_activity_;
		this->response_offset_			= src.response_offset_;
		this->server_index_				= src.server_index_;
		this->request_					= src.request_;
		this->request_started_			= src.request_started_;
		this->cgi_pid_					= src.cgi_pid_;
		this->cgi_stdin_fd_				= src.cgi_stdin_fd_;
		this->cgi_stdout_fd_			= src.cgi_stdout_fd_;
		this->cgi_output_				= src.cgi_output_;
		this->cgi_active_				= src.cgi_active_;
		this->cgi_stdin_pending_		= src.cgi_stdin_pending_;
		this->cgi_stdin_pending_offset_ = src.cgi_stdin_pending_offset_;
		this->cgi_stdin_eof_			= src.cgi_stdin_eof_;
		this->cgi_body_bytes_forwarded_ = src.cgi_body_bytes_forwarded_;
		this->cgi_body_content_length_	= src.cgi_body_content_length_;
		this->cgi_headers_sent_			= src.cgi_headers_sent_;
		this->cgi_chunked_				= src.cgi_chunked_;
		this->cgi_chunk_state_			= src.cgi_chunk_state_;
		this->cgi_chunk_remaining_		= src.cgi_chunk_remaining_;
		this->cgi_chunk_buf_			= src.cgi_chunk_buf_;
		this->cgi_send_pending_			= src.cgi_send_pending_;
		this->cgi_send_pending_offset_	= src.cgi_send_pending_offset_;
	}
	return *this;
}

Client::~Client()
{
}

int Client::get_fd() const
{
	return this->fd_;
}

const std::string &Client::get_buffer() const
{
	return this->buffer_;
}

const std::string &Client::get_response() const
{
	return this->response_;
}

time_t Client::get_last_activity() const
{
	return this->last_activity_;
}

size_t Client::get_response_offset() const
{
	return this->response_offset_;
}

size_t Client::get_server_index() const
{
	return this->server_index_;
}

HTTPRequest &Client::get_request()
{
	return this->request_;
}

bool Client::is_request_started() const
{
	return this->request_started_;
}

void Client::set_request_started(bool started)
{
	this->request_started_ = started;
}

void Client::update_activity()
{
	this->last_activity_ = time(NULL);
}

void Client::advance_response_offset(size_t bytes)
{
	this->response_offset_ += bytes;
}

void Client::reset_response_offset()
{
	this->response_offset_ = 0;
}

void Client::append_to_buffer(const char *data, size_t len)
{
	this->buffer_.append(data, len);
}

bool Client::has_complete_header() const
{
	return this->buffer_.find("\r\n\r\n") != std::string::npos;
}

void Client::clear_buffer()
{
	this->buffer_.clear();
	this->request_.reset();
	this->request_started_ = false;
}

void Client::clear_raw_buffer()
{
	this->buffer_.clear();
}

void Client::set_response(const std::string &response)
{
	this->response_		   = response;
	this->response_offset_ = 0;
}

void Client::clear_response()
{
	this->response_.clear();
	this->response_offset_ = 0;
}

void Client::start_cgi(pid_t pid, int stdin_fd, int stdout_fd)
{
	cgi_pid_	   = pid;
	cgi_stdin_fd_  = stdin_fd;
	cgi_stdout_fd_ = stdout_fd;
	cgi_active_	   = true;
	cgi_output_.clear();
	cgi_stdin_pending_.clear();
	cgi_stdin_pending_offset_ = 0;
	cgi_stdin_eof_			  = false;
	cgi_body_bytes_forwarded_ = 0;
	cgi_body_content_length_  = 0;
	cgi_headers_sent_		  = false;
	cgi_chunked_			  = false;
	cgi_chunk_state_		  = CHUNK_DEC_SIZE_;
	cgi_chunk_remaining_	  = 0;
	cgi_chunk_buf_.clear();
	cgi_send_pending_.clear();
	cgi_send_pending_offset_ = 0;
}

bool Client::is_cgi_active() const
{
	return cgi_active_;
}

int Client::get_cgi_stdout_fd() const
{
	return cgi_stdout_fd_;
}

void Client::reset_cgi_stdout_fd()
{
	cgi_stdout_fd_ = -1;
}

int Client::get_cgi_stdin_fd() const
{
	return cgi_stdin_fd_;
}

pid_t Client::get_cgi_pid() const
{
	return cgi_pid_;
}

void Client::append_cgi_output(const char *data, size_t len)
{
	cgi_output_.append(data, len);
}

const std::string &Client::get_cgi_output() const
{
	return cgi_output_;
}

void Client::close_cgi_stdin()
{
	if (cgi_stdin_fd_ != -1)
	{
		close(cgi_stdin_fd_);
		cgi_stdin_fd_ = -1;
	}
}

void Client::finish_cgi()
{
	if (cgi_stdout_fd_ != -1)
	{
		close(cgi_stdout_fd_);
		cgi_stdout_fd_ = -1;
	}
	close_cgi_stdin();
	if (cgi_pid_ != -1)
	{
		waitpid(cgi_pid_, NULL, WNOHANG);
		cgi_pid_ = -1;
	}
	cgi_active_ = false;
}

void Client::append_cgi_stdin_pending(const char *data, size_t len)
{
	cgi_stdin_pending_.append(data, len);
}

const std::string &Client::get_cgi_stdin_pending() const
{
	return cgi_stdin_pending_;
}

const char *Client::get_cgi_stdin_pending_ptr() const
{
	return cgi_stdin_pending_.c_str() + cgi_stdin_pending_offset_;
}

size_t Client::get_cgi_stdin_pending_size() const
{
	if (cgi_stdin_pending_offset_ >= cgi_stdin_pending_.size())
		return 0;
	return cgi_stdin_pending_.size() - cgi_stdin_pending_offset_;
}

bool Client::is_cgi_stdin_pending_empty() const
{
	return get_cgi_stdin_pending_size() == 0;
}

void Client::consume_cgi_stdin_pending(size_t bytes)
{
	cgi_stdin_pending_offset_ += bytes;
	// Compact once offset exceeds 1MB to reclaim memory
	static const size_t COMPACT_THRESHOLD = 1048576;
	if (cgi_stdin_pending_offset_ >= COMPACT_THRESHOLD)
	{
		if (cgi_stdin_pending_offset_ >= cgi_stdin_pending_.size())
			cgi_stdin_pending_.clear();
		else
			cgi_stdin_pending_.erase(0, cgi_stdin_pending_offset_);
		cgi_stdin_pending_offset_ = 0;
	}
}

bool Client::is_cgi_stdin_eof() const
{
	return cgi_stdin_eof_;
}

void Client::set_cgi_stdin_eof(bool val)
{
	cgi_stdin_eof_ = val;
}

size_t Client::get_cgi_body_bytes_forwarded() const
{
	return cgi_body_bytes_forwarded_;
}

void Client::add_cgi_body_bytes_forwarded(size_t n)
{
	cgi_body_bytes_forwarded_ += n;
}

size_t Client::get_cgi_body_content_length() const
{
	return cgi_body_content_length_;
}

void Client::set_cgi_body_content_length(size_t len)
{
	cgi_body_content_length_ = len;
}

bool Client::is_cgi_headers_sent() const
{
	return cgi_headers_sent_;
}

void Client::set_cgi_headers_sent(bool val)
{
	cgi_headers_sent_ = val;
}

void Client::append_cgi_send_pending(const char *data, size_t len)
{
	cgi_send_pending_.append(data, len);
}

const std::string &Client::get_cgi_send_pending() const
{
	return cgi_send_pending_;
}

const char *Client::get_cgi_send_pending_ptr() const
{
	return cgi_send_pending_.c_str() + cgi_send_pending_offset_;
}

size_t Client::get_cgi_send_pending_size() const
{
	if (cgi_send_pending_offset_ >= cgi_send_pending_.size())
		return 0;
	return cgi_send_pending_.size() - cgi_send_pending_offset_;
}

void Client::consume_cgi_send_pending(size_t bytes)
{
	cgi_send_pending_offset_ += bytes;
	// Compact once offset exceeds 1MB to reclaim memory
	static const size_t COMPACT_THRESHOLD = 1048576;
	if (cgi_send_pending_offset_ >= COMPACT_THRESHOLD)
	{
		if (cgi_send_pending_offset_ >= cgi_send_pending_.size())
			cgi_send_pending_.clear();
		else
			cgi_send_pending_.erase(0, cgi_send_pending_offset_);
		cgi_send_pending_offset_ = 0;
	}
}

void Client::set_cgi_chunked(bool val)
{
	cgi_chunked_ = val;
}

bool Client::is_cgi_chunked() const
{
	return cgi_chunked_;
}

void Client::init_cgi_chunk_decoder(int state, size_t remaining)
{
	// Map HTTPRequest::ChunkedState values to Client::ChunkedDecState.
	// HTTPRequest: CHUNK_SIZE_=0, CHUNK_DATA_=1, CHUNK_DATA_CRLF_=2,
	//              CHUNK_TRAILERS_=3
	// Client:      CHUNK_DEC_SIZE_=0, CHUNK_DEC_DATA_=1,
	//              CHUNK_DEC_DATA_CRLF_=2, CHUNK_DEC_TRAILERS_=3
	switch (state)
	{
	case 1:
		cgi_chunk_state_ = CHUNK_DEC_DATA_;
		break;
	case 2:
		cgi_chunk_state_ = CHUNK_DEC_DATA_CRLF_;
		break;
	case 3:
		cgi_chunk_state_ = CHUNK_DEC_TRAILERS_;
		break;
	default:
		cgi_chunk_state_ = CHUNK_DEC_SIZE_;
		break;
	}
	cgi_chunk_remaining_ = remaining;
	cgi_chunk_buf_.clear();
}

// Decode chunked transfer-encoding on the fly. Appends raw body bytes to
// `decoded` and sets `eof` to true when the final zero-length chunk and
// trailers have been consumed.  Returns false on a parse error.
bool Client::decode_chunked_for_cgi(const char	*data,
									size_t		 len,
									std::string &decoded,
									bool		&eof)
{
	cgi_chunk_buf_.append(data, len);
	eof = false;

	while (!cgi_chunk_buf_.empty())
	{
		if (cgi_chunk_state_ == CHUNK_DEC_SIZE_)
		{
			size_t pos = cgi_chunk_buf_.find("\r\n");
			if (pos == std::string::npos)
				return true; // need more data
			std::string line = cgi_chunk_buf_.substr(0, pos);
			cgi_chunk_buf_.erase(0, pos + 2);
			// Strip chunk-extensions (after ';')
			size_t semi = line.find(';');
			if (semi != std::string::npos)
				line = line.substr(0, semi);
			// Parse hex size
			size_t size = 0;
			for (size_t i = 0; i < line.size(); ++i)
			{
				char c = line[i];
				size *= 16;
				if (c >= '0' && c <= '9')
					size += static_cast<size_t>(c - '0');
				else if (c >= 'a' && c <= 'f')
					size += static_cast<size_t>(c - 'a' + 10);
				else if (c >= 'A' && c <= 'F')
					size += static_cast<size_t>(c - 'A' + 10);
				else
					return false; // invalid hex
			}
			cgi_chunk_remaining_ = size;
			if (size == 0)
				cgi_chunk_state_ = CHUNK_DEC_TRAILERS_;
			else
				cgi_chunk_state_ = CHUNK_DEC_DATA_;
		}
		else if (cgi_chunk_state_ == CHUNK_DEC_DATA_)
		{
			size_t avail = (cgi_chunk_buf_.size() < cgi_chunk_remaining_) ?
							   cgi_chunk_buf_.size() :
							   cgi_chunk_remaining_;
			decoded.append(cgi_chunk_buf_, 0, avail);
			cgi_chunk_buf_.erase(0, avail);
			cgi_chunk_remaining_ -= avail;
			if (cgi_chunk_remaining_ == 0)
				cgi_chunk_state_ = CHUNK_DEC_DATA_CRLF_;
			else
				return true; // need more data
		}
		else if (cgi_chunk_state_ == CHUNK_DEC_DATA_CRLF_)
		{
			if (cgi_chunk_buf_.size() < 2)
				return true; // need more data
			if (cgi_chunk_buf_[0] != '\r' || cgi_chunk_buf_[1] != '\n')
				return false; // protocol error
			cgi_chunk_buf_.erase(0, 2);
			cgi_chunk_state_ = CHUNK_DEC_SIZE_;
		}
		else if (cgi_chunk_state_ == CHUNK_DEC_TRAILERS_)
		{
			size_t pos = cgi_chunk_buf_.find("\r\n");
			if (pos == std::string::npos)
				return true; // need more data
			if (pos == 0)
			{
				// Empty line — end of trailers
				cgi_chunk_buf_.erase(0, 2);
				cgi_chunk_state_ = CHUNK_DEC_DONE_;
				eof				 = true;
				return true;
			}
			// Skip trailer line
			cgi_chunk_buf_.erase(0, pos + 2);
		}
		else // CHUNK_DEC_DONE_
		{
			eof = true;
			return true;
		}
	}
	return true;
}
