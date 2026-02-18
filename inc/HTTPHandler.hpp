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

class HTTPHandler
{
  private:
	static std::map<int, std::string> error_pages_;

	static std::string				  get_mime_type(const std::string &path);
	static std::string				  url_decode(const std::string &value);

	///////////////////////////////////////////////////////////////// Methods //
	static HTTPResponse				  handle_get(const HTTPRequest &request,
												 const RouteConfig &route);
	static HTTPResponse				  handle_post(const HTTPRequest &request,
												  const RouteConfig &route);
	static HTTPResponse				  handle_delete(const HTTPRequest &request,
													const RouteConfig &route);

  public:
	///////////////////////////////////////////////// Canonical Orthodox Form //
	HTTPHandler();
	HTTPHandler(const HTTPHandler &other);
	HTTPHandler &operator=(const HTTPHandler &other);
	~HTTPHandler();

	///////////////////////////////////////////////////////////// Error pages //
	static void		   set_error_pages(const std::map<int, std::string> &pages);
	static std::string get_error_page(int status_code);

	///////////////////////////////////////////////////////////////// Request //
	static HTTPResponse handle_request(const HTTPRequest  &request,
									   const RouteConfig  *route,
									   const ServerConfig &server);
};

#endif
