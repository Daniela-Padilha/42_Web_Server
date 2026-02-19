#include "../inc/Client.hpp"

Client::Client(int fd) :
	fd_(fd),
	last_activity_(time(NULL)),
	response_offset_(0)
{
}

Client::Client(const Client &src) :
	fd_(src.fd_),
	buffer_(src.buffer_),
	response_(src.response_),
	last_activity_(src.last_activity_),
	response_offset_(src.response_offset_)
{
}

Client &Client::operator=(const Client &src)
{
	if (this != &src)
	{
		this->fd_			   = src.fd_;
		this->buffer_		   = src.buffer_;
		this->response_		   = src.response_;
		this->last_activity_   = src.last_activity_;
		this->response_offset_ = src.response_offset_;
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
