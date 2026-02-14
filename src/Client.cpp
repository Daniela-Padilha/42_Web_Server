#include "../inc/Client.hpp"

Client::Client(int fd) : fd_(fd)
{
}

Client::Client(const Client &src) :
	fd_(src.fd_),
	buffer_(src.buffer_),
	response_(src.response_)
{
}

Client &Client::operator=(const Client &src)
{
	if (this != &src)
	{
		this->fd_		= src.fd_;
		this->buffer_	= src.buffer_;
		this->response_ = src.response_;
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
	this->response_ = response;
}

void Client::clear_response()
{
	this->response_.clear();
}
