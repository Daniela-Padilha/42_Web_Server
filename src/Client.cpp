#include "../inc/Client.hpp"

Client::Client(int fd) : fd_(fd), buffer_("")
{
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

void Client::append_to_buffer(const char *data, size_t len)
{
	this->buffer_.append(data, len);
}

bool Client::has_complete_header() const
{
	if (this->buffer_.find("\r\n\r\n") != std::string::npos)
		return true;
	else
		return false;
}

void Client::clear_buffer()
{
	this->buffer_.clear();
}
