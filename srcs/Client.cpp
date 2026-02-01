#include "../inc/Client.hpp"

Client::Client(int fd): _fd(fd), _buffer("") {}

Client::~Client() {}

int Client::getFd() const {
	return this->_fd;
}

const std::string& Client::getBuffer() const {
	return this->_buffer;
}

void Client::appendToBuffer(const char* data, size_t len) {
	this->_buffer.append(data, len);
}

bool Client::hasCompleteHeader() const {
	if (this->_buffer.find("\r\n\r\n") != std::string::npos)
		return true;
	else
		return false;
}

void Client::clearBuffer() {
	this->_buffer.clear();
}
