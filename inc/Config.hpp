#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "../inc/utils_print.hpp"

struct RouteConfig
{
	std::string				 path;
	std::vector<std::string> allowed_methods;
	std::string				 root;
	std::string				 index;
	bool					 autoindex;
	std::string				 upload_store;
	std::string				 redirect_url;
	int						 redirect_code;
	std::string				 cgi_extension;
	std::string				 cgi_path;
	size_t					 client_max_body_size;
	bool					 has_client_max_body_size;

	RouteConfig();
};

struct ServerConfig
{
	std::string				   host;
	int						   port;
	bool					   has_listen_;
	std::vector<std::string>   server_names;
	std::map<int, std::string> error_pages;
	size_t					   client_max_body_size;
	std::vector<RouteConfig>   routes;

	ServerConfig();
};

class Config
{
  private:
	std::vector<ServerConfig>		servers_;
	std::string						error_;

	/////////////////////////////////////////////////////////// Parsing utils //
	static std::vector<std::string> tokenize(const std::string &line);
	static std::string				clean_line(const std::string &line);

	bool parse_server_block(std::ifstream &file, ServerConfig &srv);
	bool parse_location_block(std::ifstream		&file,
							  const std::string &path,
							  RouteConfig		&out);
	bool parse_directive(ServerConfig					&srv,
						 const std::vector<std::string> &words);
	bool parse_route_directive(RouteConfig					  &route,
							   const std::vector<std::string> &words);
	bool parse_autoindex(RouteConfig					&route,
						 const std::vector<std::string> &words);
	bool parse_allow_methods(RouteConfig					&route,
							 const std::vector<std::string> &words);
	bool parse_return(RouteConfig					 &route,
					  const std::vector<std::string> &words);
	bool parse_route_string_directive(const std::vector<std::string> &words,
									  std::string					 &out,
									  const std::string &directive);

	////////////////////////////////////////////////////////////// Validation //
	bool validate();
	bool parse_size(const std::string &value, size_t &out);
	bool parse_listen(const std::string &value, ServerConfig &srv);

	///////////////////////////////////////////////////////////////// Helpers //
	static bool read_next_line(std::ifstream &file, std::string &out);
	bool expect_open_brace(std::ifstream &file, const std::string &context);
	bool expect_args(const std::vector<std::string> &words,
					 size_t							 min,
					 const std::string				&directive);

  public:
	///////////////////////////////////////////////// Canonical Orthodox Form //
	Config();
	Config(const Config &src);
	Config &operator=(const Config &src);
	~Config();

	////////////////////////////////////////////////////////////////// Public //
	bool							 parse(const std::string &filepath);
	const std::vector<ServerConfig> &get_server() const;
	const std::string				&error() const;
};

#endif
