#include "../inc/Client.hpp"

Client::Client(int fd_): fd(fd_), buffer("") {}

Client::~Client() {}