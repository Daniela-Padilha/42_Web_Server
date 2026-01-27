#pragma once

#include <iostream>
#include <cstring>
#include <sys/socket.h> //socket
#include <netinet/in.h> //sockaddr_in
#include <unistd.h> //close
#include <cerrno> //errno

int socket_creator(void);
