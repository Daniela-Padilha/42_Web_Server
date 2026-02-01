#include "../inc/Client.hpp"

Client::Client(int fd_): fd(fd_), buffer("") {}

Client::~Client() {}

bool Client::hasCompleteHeader(const std::string& buffer) {
	if (buffer.find("\r\n\r\n") != std::string::npos)
		return true;
	else
		return false;
}
