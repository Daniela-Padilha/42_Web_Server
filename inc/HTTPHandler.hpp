#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "../inc/HTTPRequest.hpp"
#include "../inc/HTTPResponse.hpp"
#include "../inc/utils_print.hpp"

class HTTPHandler
{
  private:
	static std::string	get_mime_type(const std::string &path);
	static std::string	url_decode(const std::string &value);

	///////////////////////////////////////////////////////////////// Methods //
	static HTTPResponse handle_get(const HTTPRequest &request);
	static HTTPResponse handle_post(const HTTPRequest &request);
	static HTTPResponse handle_delete(const HTTPRequest &request);

  public:
	///////////////////////////////////////////////// Canonical Orthodox Form //
	HTTPHandler();
	HTTPHandler(const HTTPHandler &other);
	HTTPHandler &operator=(const HTTPHandler &other);
	~HTTPHandler();

	///////////////////////////////////////////////////////////////// Request //
	static HTTPResponse handle_request(const HTTPRequest &request);
};

#endif
