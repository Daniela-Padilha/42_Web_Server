#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "../inc/Config.hpp"
#include "../inc/HTTPRequest.hpp"
#include "../inc/HTTPResponse.hpp"
#include "../inc/utils_print.hpp"
#include "CGI.hpp"

class HTTPHandler
{
  private:
	static std::string	get_mime_type(const std::string &path);
	static std::string	url_decode(const std::string &value);

	///////////////////////////////////////////////////////////////// Methods //
	static HTTPResponse handle_get(const HTTPRequest  &request,
								   const RouteConfig  &route,
								   const ServerConfig &server);
	static HTTPResponse handle_post(const HTTPRequest &request,
									const RouteConfig &route);
	static HTTPResponse handle_delete(const HTTPRequest	 &request,
									  const RouteConfig	 &route,
									  const ServerConfig &server);
	static HTTPResponse generate_autoindex(const std::string  &path,
										   const std::string  &uri,
										   const ServerConfig &server);

  public:
	///////////////////////////////////////////////// Canonical Orthodox Form //
	HTTPHandler();
	HTTPHandler(const HTTPHandler &other);
	HTTPHandler &operator=(const HTTPHandler &other);
	~HTTPHandler();

	///////////////////////////////////////////////////////////// Error pages //
	static std::string	get_error_page(int				   status_code,
									   const ServerConfig &server);

	///////////////////////////////////////////////////////////////// Request //
	static HTTPResponse handle_request(const HTTPRequest  &request,
									   const RouteConfig  *route,
									   const ServerConfig &server);

	///////////////////////////////////////////////////////////// CGI parsing //
	static HTTPResponse parse_cgi_response(const std::string  &raw,
										   const ServerConfig &server);
	// Parse only CGI headers (no body); used for streaming CGI output.
	// Returns an HTTPResponse with status + headers set, but no body.
	static HTTPResponse parse_cgi_headers(const std::string &cgi_header_section,
										  const ServerConfig &server);
};

#endif
