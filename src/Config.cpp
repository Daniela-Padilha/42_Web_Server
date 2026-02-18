#include "../inc/Config.hpp"

static const size_t BYTES_PER_KB		  = 1024;
static const size_t BYTES_PER_MB		  = static_cast<size_t>(1024) * 1024;
static const size_t DEFAULT_MAX_BODY_SIZE = static_cast<size_t>(1024) * 1024;
static const int	DEFAULT_PORT		  = 8080;
static const int	MIN_PORT			  = 1;
static const int	MAX_PORT			  = 65535;

///////////////////////////////////////////////////////////////// RouteConfig //
RouteConfig::RouteConfig() :
	path("/"),
	index("index.html"),
	autoindex(false),
	redirect_code(0)
{
}

//////////////////////////////////////////////////////////////// ServerConfig //
ServerConfig::ServerConfig() :
	host("0.0.0.0"),
	port(DEFAULT_PORT),
	client_max_body_size(DEFAULT_MAX_BODY_SIZE)
{
}

///////////////////////////////////////////////////// Canonical Orthodox Form //
Config::Config()
{
}

Config::Config(const Config &src) : server_(src.server_), error_(src.error_)
{
}

Config &Config::operator=(const Config &src)
{
	if (this != &src)
	{
		server_ = src.server_;
	}
	return *this;
}

Config::~Config()
{
}

////////////////////////////////////////////////////////////// Parseing utils //
std::string Config::clean_line(const std::string &line)
{
	std::string result = line;
	size_t		hash   = result.find('#');
	if (hash != std::string::npos)
	{
		result = result.substr(0, hash);
	}
	size_t start = result.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
	{
		return "";
	}
	size_t end = result.find_last_not_of(" \t\r\n");
	return result.substr(start, end - start + 1);
}

std::vector<std::string> Config::tokenize(const std::string &line)
{
	std::string clean = line;
	if (!clean.empty() && clean[clean.size() - 1] == ';')
	{
		clean = clean.substr(0, clean.size() - 1);
	}

	std::vector<std::string> words;
	std::istringstream		 stream(clean);
	std::string				 word;
	while ((stream >> word) != 0)
	{
		words.push_back(word);
	}
	return words;
}

bool Config::expect_args(const std::vector<std::string> &words,
						 size_t							 min,
						 const std::string				&directive)
{
	if (words.size() >= min)
	{
		return true;
	}
	error_ = directive + " requires a value";
	return false;
}

bool Config::read_next_line(std::ifstream &file, std::string &out)
{
	std::string line;
	while (std::getline(file, line) != 0)
	{
		out = clean_line(line);
		if (!out.empty())
		{
			return true;
		}
	}
	return false;
}

bool Config::expect_open_brace(std::ifstream &file, const std::string &context)
{
	std::string line;
	if (!read_next_line(file, line))
	{
		error_ = "expected '{' after '" + context + "'";
		return false;
	}
	if (line != "{")
	{
		error_ = "expected '{' after '" + context + "', got '" + line + "'";
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////////// Public //
bool Config::parse(const std::string &filepath)
{
	dprint("Config::parse: opening file: " << filepath);
	std::ifstream file(filepath.c_str());
	if (!file.is_open())
	{
		error_ = "cannot open config file: " + filepath;
		eprint("Config::parse: " << error_);
		return false;
	}

	std::string trimmed;
	bool		found_server = false;

	while (read_next_line(file, trimmed))
	{
		std::vector<std::string> words = tokenize(trimmed);

		if (words[0] == "server")
		{
			if (found_server)
			{
				error_ = "only one server block is supported";
				eprint("Config::parse: " << error_);
				return false;
			}
			if (words.size() != 1)
			{
				error_ = "expected 'server' alone on its line";
				eprint("Config::parse: " << error_);
				return false;
			}
			if (!expect_open_brace(file, "server"))
			{
				return false;
			}

			dprint("Config::parse: entering server block");
			if (!parse_server_block(file))
			{
				return false;
			}
			found_server = true;
			dprint("Config::parse: server block parsed successfully");
		}
		else
		{
			error_ = "expected 'server' block, got '" + words[0] + "'";
			eprint("Config::parse: " << error_);
			return false;
		}
	}

	if (!found_server)
	{
		error_ = "config file is empty";
		eprint("Config::parse: " << error_);
		return false;
	}

	dprint("Config::parse: validating configuration");
	return validate();
}

const ServerConfig &Config::get_server() const
{
	return server_;
}

const std::string &Config::error() const
{
	return error_;
}

//////////////////////////////////////////////////////////////// Server Block //
bool Config::parse_server_block(std::ifstream &file)
{
	ServerConfig srv;
	std::string	 trimmed;

	while (read_next_line(file, trimmed))
	{
		if (trimmed == "}")
		{
			server_ = srv;
			dprint("Config::parse_server_block: block closed");
			return true;
		}

		std::vector<std::string> words = tokenize(trimmed);

		if (words[0] == "location")
		{
			if (words.size() != 2)
			{
				error_ = "location requires exactly a path";
				eprint("Config::parse_server_block: " << error_);
				return false;
			}
			std::string loc_path = words[1];

			if (!expect_open_brace(file, "location"))
			{
				return false;
			}

			dprint("Config::parse_server_block: entering location "
				   << loc_path);
			RouteConfig route;
			if (!parse_location_block(file, loc_path, route))
			{
				return false;
			}
			srv.routes.push_back(route);
			dprint("Config::parse_server_block: location " << loc_path
														   << " parsed");
		}
		else
		{
			if (!parse_directive(srv, words))
			{
				return false;
			}
		}
	}

	error_ = "unexpected end of config: missing '}'";
	eprint("Config::parse_server_block: " << error_);
	return false;
}

bool Config::parse_directive(ServerConfig					&srv,
							 const std::vector<std::string> &words)
{
	const std::string &key = words[0];

	if (key == "listen")
	{
		if (!expect_args(words, 2, "listen"))
		{
			return false;
		}
		dprint("Config::parse_directive: listen " << words[1]);
		return parse_listen(words[1], srv);
	}
	if (key == "server_name")
	{
		if (!expect_args(words, 2, "server_name"))
		{
			return false;
		}
		for (size_t i = 1; i < words.size(); i++)
		{
			srv.server_names.push_back(words[i]);
			dprint("Config::parse_directive: server_name " << words[i]);
		}
		return true;
	}
	if (key == "error_page")
	{
		if (words.size() < 3)
		{
			error_ = "error_page requires a code and a path";
			eprint("Config::parse_directive: " << error_);
			return false;
		}
		int				   code = 0;
		std::istringstream code_stream(words[1]);
		if (!(code_stream >> code))
		{
			error_ = "invalid error code: '" + words[1] + "'";
			eprint("Config::parse_directive: " << error_);
			return false;
		}
		srv.error_pages[code] = words[2];
		dprint("Config::parse_directive: error_page " << code << " -> "
													  << words[2]);
		return true;
	}
	if (key == "client_max_body_size")
	{
		if (!expect_args(words, 2, "client_max_body_size"))
		{
			return false;
		}
		dprint("Config::parse_directive: client_max_body_size " << words[1]);
		return parse_size(words[1], srv.client_max_body_size);
	}
	error_ = "unknown directive: '" + key + "'";
	eprint("Config::parse_directive: " << error_);
	return false;
}

////////////////////////////////////////////////////////////// Location Block //
bool Config::parse_location_block(std::ifstream		&file,
								  const std::string &path,
								  RouteConfig		&out)
{
	out		 = RouteConfig();
	out.path = path;

	std::string trimmed;
	while (read_next_line(file, trimmed))
	{
		if (trimmed == "}")
		{
			dprint("Config::parse_location_block: " << path << " block closed");
			return true;
		}

		std::vector<std::string> words = tokenize(trimmed);

		if (!parse_route_directive(out, words))
		{
			return false;
		}
	}

	error_ = "unexpected end of config: missing '}'";
	eprint("Config::parse_location_block: " << error_);
	return false;
}

bool Config::parse_route_directive(RouteConfig					  &route,
								   const std::vector<std::string> &words)
{
	const std::string &key = words[0];

	if (key == "root")
	{
		if (!expect_args(words, 2, "root"))
		{
			return false;
		}
		route.root = words[1];
		dprint("Config::parse_route_directive: root " << words[1]);
	}
	else if (key == "index")
	{
		if (!expect_args(words, 2, "index"))
		{
			return false;
		}
		route.index = words[1];
		dprint("Config::parse_route_directive: index " << words[1]);
	}
	else if (key == "autoindex")
	{
		if (!expect_args(words, 2, "autoindex"))
		{
			return false;
		}
		if (words[1] == "on")
		{
			route.autoindex = true;
		}
		else if (words[1] == "off")
		{
			route.autoindex = false;
		}
		else
		{
			error_ = "autoindex must be 'on' or 'off', got '" + words[1] + "'";
			eprint("Config::parse_route_directive: " << error_);
			return false;
		}
		dprint("Config::parse_route_directive: autoindex " << words[1]);
	}
	else if (key == "allow_methods")
	{
		if (!expect_args(words, 2, "allow_methods"))
		{
			return false;
		}
		route.allowed_methods.clear();
		for (size_t i = 1; i < words.size(); i++)
		{
			route.allowed_methods.push_back(words[i]);
		}
		dprint("Config::parse_route_directive: allow_methods set ("
			   << route.allowed_methods.size() << " methods)");
	}
	else if (key == "return")
	{
		if (words.size() < 3)
		{
			error_ = "return requires a code and a URL";
			eprint("Config::parse_route_directive: " << error_);
			return false;
		}
		std::istringstream code_stream(words[1]);
		if (!(code_stream >> route.redirect_code))
		{
			error_ = "invalid redirect code: '" + words[1] + "'";
			eprint("Config::parse_route_directive: " << error_);
			return false;
		}
		route.redirect_url = words[2];
		dprint("Config::parse_route_directive: return "
			   << route.redirect_code << " " << route.redirect_url);
	}
	else if (key == "upload_store")
	{
		if (!expect_args(words, 2, "upload_store"))
		{
			return false;
		}
		route.upload_store = words[1];
		dprint("Config::parse_route_directive: upload_store " << words[1]);
	}
	else if (key == "cgi_extension")
	{
		if (!expect_args(words, 2, "cgi_extension"))
		{
			return false;
		}
		route.cgi_extension = words[1];
		dprint("Config::parse_route_directive: cgi_extension " << words[1]);
	}
	else if (key == "cgi_path")
	{
		if (!expect_args(words, 2, "cgi_path"))
		{
			return false;
		}
		route.cgi_path = words[1];
		dprint("Config::parse_route_directive: cgi_path " << words[1]);
	}
	else
	{
		error_ = "unknown directive in location block: '" + key + "'";
		eprint("Config::parse_route_directive: " << error_);
		return false;
	}
	return true;
}

////////////////////////////////////////////////////////////////// Validation //
bool Config::parse_listen(const std::string &value, ServerConfig &srv)
{
	size_t colon = value.find(':');
	if (colon != std::string::npos)
	{
		srv.host					= value.substr(0, colon);
		std::string		   port_str = value.substr(colon + 1);
		std::istringstream port_stream(port_str);
		if (!(port_stream >> srv.port))
		{
			error_ = "invalid port: '" + port_str + "'";
			eprint("Config::parse_listen: " << error_);
			return false;
		}
	}
	else
	{
		std::istringstream port_stream(value);
		if (!(port_stream >> srv.port))
		{
			error_ = "invalid port: '" + value + "'";
			eprint("Config::parse_listen: " << error_);
			return false;
		}
	}
	dprint("Config::parse_listen: host=" << srv.host << " port=" << srv.port);
	return true;
}

bool Config::parse_size(const std::string &value, size_t &out)
{
	if (value.empty())
	{
		error_ = "empty body size value";
		eprint("Config::parse_size: " << error_);
		return false;
	}

	char		suffix	   = value[value.size() - 1];
	size_t		multiplier = 1;
	std::string number_str = value;

	if (suffix == 'K' || suffix == 'k')
	{
		multiplier = BYTES_PER_KB;
		number_str = value.substr(0, value.size() - 1);
	}
	else if (suffix == 'M' || suffix == 'm')
	{
		multiplier = BYTES_PER_MB;
		number_str = value.substr(0, value.size() - 1);
	}

	std::istringstream num_stream(number_str);
	if (!(num_stream >> out))
	{
		error_ = "invalid body size: '" + value + "'";
		eprint("Config::parse_size: " << error_);
		return false;
	}

	out *= multiplier;
	dprint("Config::parse_size: " << value << " -> " << out << " bytes");
	return true;
}

bool Config::validate()
{
	dprint("Config::validate: starting validation");
	if (server_.port < MIN_PORT || server_.port > MAX_PORT)
	{
		error_ = "port must be between 1 and 65535";
		eprint("Config::validate: " << error_);
		return false;
	}

	for (size_t i = 0; i < server_.routes.size(); i++)
	{
		const RouteConfig &route = server_.routes[i];

		if (route.path.empty() || route.path[0] != '/')
		{
			error_ = "route path must start with '/': '" + route.path + "'";
			eprint("Config::validate: " << error_);
			return false;
		}

		if (route.redirect_code != 0 && route.redirect_code != 301
			&& route.redirect_code != 302)
		{
			std::ostringstream msg;
			msg << "redirect code must be 301 or 302, got "
				<< route.redirect_code;
			error_ = msg.str();
			eprint("Config::validate: " << error_);
			return false;
		}
	}
	dprint("Config::validate: configuration is valid");
	return true;
}
