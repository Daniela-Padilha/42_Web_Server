#include "../inc/HTTPHandler.hpp"

static const std::string BOUNDARY_PREFIX	= "boundary=";
static const std::string FILENAME_PREFIX	= "filename=\"";
static const std::string HEADER_BODY_DELIM	= "\r\n\r\n";
static const std::string DEFAULT_ROOT_DIR	= "files";
static const std::string DEFAULT_INDEX_FILE = "index.html";
static const std::string DEFAULT_UPLOAD_DIR = "uploads";

/////////////////////////////////////////////////////////////// URI utilities //
static std::string		 strip_query_string(const std::string &uri)
{
	size_t pos = uri.find('?');
	if (pos != std::string::npos)
	{
		return uri.substr(0, pos);
	}
	return uri;
}

static std::string strip_route_prefix(const std::string &uri,
									  const std::string &route_path)
{
	if (route_path == "/" || route_path.empty())
	{
		return uri;
	}
	if (uri.compare(0, route_path.size(), route_path) == 0)
	{
		std::string stripped = uri.substr(route_path.size());
		if (stripped.empty() || stripped[0] != '/')
		{
			stripped = "/" + stripped;
		}
		return stripped;
	}
	return uri;
}

static void process_path_segment(std::vector<std::string> &segments,
								 const std::string		  &segment)
{
	if (segment == "..")
	{
		if (!segments.empty())
		{
			segments.pop_back();
		}
	}
	else if (segment != ".")
	{
		segments.push_back(segment);
	}
}

static std::string sanitize_path(const std::string &path)
{
	std::vector<std::string> segments;
	std::string				 segment;
	for (size_t i = 0; i < path.size(); ++i)
	{
		if (path[i] == '/')
		{
			if (!segment.empty())
			{
				process_path_segment(segments, segment);
				segment.clear();
			}
		}
		else
		{
			segment += path[i];
		}
	}
	if (!segment.empty())
	{
		process_path_segment(segments, segment);
	}
	std::string result = "/";
	for (size_t i = 0; i < segments.size(); i++)
	{
		result += segments[i];
		if (i + 1 < segments.size())
		{
			result += "/";
		}
	}
	return result;
}

std::string HTTPHandler::get_error_page(int					status_code,
										const ServerConfig &server)
{
	std::map<int, std::string>::const_iterator it
		= server.error_pages.find(status_code);
	if (it != server.error_pages.end())
	{
		return it->second;
	}
	return "";
}

HTTPResponse HTTPHandler::handle_request(const HTTPRequest	&request,
										 const RouteConfig	*route,
										 const ServerConfig &server)
{
	const std::string &method = request.get_method();

	dprint("HTTPHandler: Handling " << method << " request for "
									<< request.get_request_target());

	if (route == NULL)
	{
		dprint("HTTPHandler: handle_request: no route match");
		return HTTPResponse::error_404(get_error_page(404, server));
	}

	// handle redirection
	if (route->redirect_code != 0)
	{
		dprint("HTTPHandler: Redirecting to " << route->redirect_url << " ("
											  << route->redirect_code << ")");
		if (route->redirect_code == 301)
		{
			return HTTPResponse::redirect_301(route->redirect_url);
		}
		if (route->redirect_code == 302)
		{
			return HTTPResponse::redirect_302(route->redirect_url);
		}
	}

	// check allowed methods
	if (!route->allowed_methods.empty())
	{
		bool method_allowed = false;
		for (size_t i = 0; i < route->allowed_methods.size(); i++)
		{
			if (route->allowed_methods[i] == method)
			{
				method_allowed = true;
				break;
			}
		}
		if (!method_allowed)
		{
			return HTTPResponse::error_405(get_error_page(405, server));
		}
	}

	// enforce client_max_body_size (route-level overrides server)
	size_t max_body = server.client_max_body_size;
	if (route->has_client_max_body_size)
	{
		max_body = route->client_max_body_size;
	}
	std::string content_length_header
		= request.get_header_value("Content-Length");
	if (!content_length_header.empty() && max_body > 0)
	{
		size_t content_length
			= static_cast<size_t>(std::atol(content_length_header.c_str()));
		if (content_length > max_body)
		{
			return HTTPResponse::error_413(get_error_page(413, server));
		}
	}

	// CGI dispatch: if the route has a cgi_extension configured and the
	// request URI ends with that extension, hand off to the CGI executor
	if (!route->cgi_extension.empty())
	{
		std::string request_target
			= strip_query_string(request.get_request_target());
		size_t ext_len = route->cgi_extension.size();
		bool   matches
			= request_target.size() >= ext_len
			  && request_target.compare(request_target.size() - ext_len,
										ext_len,
										route->cgi_extension)
					 == 0;
		if (matches)
		{
			dprint("HTTPHandler: CGI dispatch for " << request_target);
			CGI			cgi(request, *route);
			std::string raw = cgi.execute();
			if (raw.empty())
			{
				return HTTPResponse::error_500(get_error_page(500, server));
			}
			return parse_cgi_response(raw, server);
		}
	}

	if (method == "GET")
	{
		return handle_get(request, *route, server);
	}
	if (method == "HEAD")
	{
		HTTPResponse resp = handle_get(request, *route, server);
		resp.clear_body();
		return resp;
	}
	if (method == "POST")
	{
		return handle_post(request, *route);
	}
	if (method == "DELETE")
	{
		return handle_delete(request, *route, server);
	}

	return HTTPResponse::error_405(get_error_page(405, server));
}

HTTPResponse HTTPHandler::generate_autoindex(const std::string	&path,
											 const std::string	&uri,
											 const ServerConfig &server)
{
	DIR			  *dir = NULL;
	struct dirent *ent = NULL;
	dir				   = opendir(path.c_str());
	if (dir == NULL)
	{
		dprint("HTTPHandler: Failed to open directory for autoindex: " << path);
		return HTTPResponse::error_404(get_error_page(404, server));
	}

	std::string body = "<html>\n<head><title>Index of " + uri
					   + "</title></head>\n"
						 "<body>\n<h1>Index of "
					   + uri + "</h1><hr><pre>\n";

	while ((ent = readdir(dir)) != NULL)
	{
		std::string name = ent->d_name;
		if (name == ".")
		{
			continue;
		}

		std::string full_path = path;
		if (full_path[full_path.size() - 1] != '/')
		{
			full_path += "/";
		}
		full_path += name;

		struct stat stt;
		std::string display_name = name;
		if (stat(full_path.c_str(), &stt) == 0 && S_ISDIR(stt.st_mode))
		{
			display_name += "/";
		}

		body += "<a href=\"" + name;
		body += (S_ISDIR(stt.st_mode) ? "/" : "");
		body += "\">" + display_name + "</a>\n";
	}
	closedir(dir);

	body += "</pre><hr></body>\n</html>";
	return HTTPResponse::success_200(body, "text/html");
}

HTTPResponse HTTPHandler::handle_get(const HTTPRequest	&request,
									 const RouteConfig	&route,
									 const ServerConfig &server)
{
	std::string request_target
		= strip_query_string(request.get_request_target());
	request_target = strip_route_prefix(request_target, route.path);
	request_target = sanitize_path(request_target);
	std::string root_dir;
	if (route.root.empty())
	{
		root_dir = DEFAULT_ROOT_DIR;
	}
	else
	{
		root_dir = route.root;
	}

	std::string index_file;
	if (route.index.empty())
	{
		index_file = DEFAULT_INDEX_FILE;
	}
	else
	{
		index_file = route.index;
	}

	std::string file_path = root_dir + request_target;

	dprint("HTTPHandler: GET file_path = " << file_path);

	struct stat file_stat;
	if (stat(file_path.c_str(), &file_stat) < 0)
	{
		dprint("HTTPHandler: File not found: " << file_path);
		return HTTPResponse::error_404(get_error_page(404, server));
	}

	if (S_ISDIR(file_stat.st_mode))
	{
		std::string index_path = file_path + "/" + index_file;
		if (stat(index_path.c_str(), &file_stat) == 0)
		{
			file_path = index_path;
		}
		else if (route.autoindex)
		{
			return generate_autoindex(file_path,
									  strip_query_string(
										  request.get_request_target()),
									  server);
		}
		else
		{
			return HTTPResponse::error_404(get_error_page(404, server));
		}
	}

	if (access(file_path.c_str(), F_OK) != 0)
		return HTTPResponse::error_404(get_error_page(404, server));

	if (access(file_path.c_str(), R_OK) != 0)
		return HTTPResponse::error_403(get_error_page(403, server));
	std::ifstream file(file_path.c_str(), std::ios::binary);
	if (!file)
	{
		dprint("HTTPHandler: Cannot open file: " << file_path);
		return HTTPResponse::error_500(get_error_page(500, server));
	}

	std::string body((std::istreambuf_iterator<char>(file)),
					 std::istreambuf_iterator<char>());
	file.close();

	std::string mime_type = get_mime_type(file_path);

	dprint("HTTPHandler: Serving file: " << file_path << " (" << body.size()
										 << " bytes, " << mime_type << ")");

	return HTTPResponse::success_200(body, mime_type);
}

HTTPResponse HTTPHandler::handle_post(const HTTPRequest &request,
									  const RouteConfig &route)
{
	const std::string &content_type = request.get_header_value("content-type");
	const std::string &body			= request.get_body();
	std::string		   uploads_dir;
	if (route.upload_store.empty())
	{
		uploads_dir = DEFAULT_UPLOAD_DIR;
	}
	else
	{
		uploads_dir = route.upload_store;
	}

	dprint("HTTPHandler: POST content-type = " << content_type);

	if (content_type.find("multipart/form-data") != std::string::npos)
	{
		size_t boundary_pos = content_type.find("boundary=");
		if (boundary_pos != std::string::npos)
		{
			std::string boundary
				= content_type.substr(boundary_pos + BOUNDARY_PREFIX.length());
			boundary = "--" + boundary;

			size_t filename_pos = body.find(FILENAME_PREFIX);
			if (filename_pos != std::string::npos)
			{
				size_t filename_start = filename_pos + FILENAME_PREFIX.length();
				size_t filename_end	  = body.find('\"', filename_start);
				std::string filename
					= body.substr(filename_start,
								  filename_end - filename_start);

				size_t data_start = body.find(HEADER_BODY_DELIM, filename_end);
				if (data_start != std::string::npos)
				{
					data_start += HEADER_BODY_DELIM.length();

					size_t data_end
						= body.find("\r\n--" + boundary.substr(2), data_start);
					if (data_end == std::string::npos)
					{
						data_end = body.length();
					}

					std::string file_data
						= body.substr(data_start, data_end - data_start);

					std::string	  upload_path = uploads_dir + "/" + filename;
					std::ofstream outfile(upload_path.c_str(),
										  std::ios::binary);
					if (outfile != 0)
					{
						outfile << file_data;
						outfile.close();
						dprint("HTTPHandler: File uploaded: " << upload_path);

						std::ostringstream response_body;
						response_body
							<< "<html><body><h1>201 Created</h1><p>File "
							   "uploaded: "
							<< filename << "</p></body></html>";
						return HTTPResponse::success_201(response_body.str(),
														 "text/html");
					}
				}
			}
		}
	}
	else if (content_type.find("application/x-www-form-urlencoded")
			 != std::string::npos)
	{
		std::string	  decoded_body = url_decode(body);

		std::string	  upload_path = uploads_dir + "/form_data.txt";
		std::ofstream outfile(upload_path.c_str(), std::ios::app);
		if (outfile != 0)
		{
			outfile << decoded_body << "\n";
			outfile.close();
			dprint("HTTPHandler: Form data saved to: " << upload_path);

			std::ostringstream response_body;
			response_body << "<html><body><h1>201 Created</h1><p>Data "
							 "received: "
						  << decoded_body << "</p></body></html>";
			return HTTPResponse::success_201(response_body.str(), "text/html");
		}
	}

	std::ostringstream response_body;
	response_body << "<html><body><h1>200 OK</h1><p>Body received: "
				  << body.size() << " bytes</p></body></html>";
	return HTTPResponse::success_200(response_body.str(), "text/html");
}

HTTPResponse HTTPHandler::handle_delete(const HTTPRequest  &request,
										const RouteConfig  &route,
										const ServerConfig &server)
{
	std::string request_target
		= strip_query_string(request.get_request_target());
	request_target = strip_route_prefix(request_target, route.path);
	request_target = sanitize_path(request_target);
	std::string root_dir;
	if (route.root.empty())
	{
		root_dir = DEFAULT_ROOT_DIR;
	}
	else
	{
		root_dir = route.root;
	}

	if (request_target == "/")
	{
		return HTTPResponse::error_400(get_error_page(400, server));
	}

	std::string file_path = root_dir + request_target;

	dprint("HTTPHandler: DELETE file_path = " << file_path);

	struct stat file_stat;
	if (stat(file_path.c_str(), &file_stat) < 0)
	{
		dprint("HTTPHandler: File not found for deletion: " << file_path);
		return HTTPResponse::error_404(get_error_page(404, server));
	}

	if (S_ISDIR(file_stat.st_mode))
	{
		return HTTPResponse::error_400(get_error_page(400, server));
	}

	if (access(file_path.c_str(), F_OK) != 0)
		return HTTPResponse::error_404(get_error_page(404, server));

	if (access(file_path.c_str(), W_OK) != 0)
		return HTTPResponse::error_403(get_error_page(403, server));
	if (unlink(file_path.c_str()) < 0)
	{
		dprint("HTTPHandler: Failed to delete file: " << strerror(errno));
		return HTTPResponse::error_500(get_error_page(500, server));
	}

	dprint("HTTPHandler: File deleted: " << file_path);

	std::string body = "<html><body><h1>200 OK</h1><p>File deleted: "
					   + request_target + "</p></body></html>";
	return HTTPResponse::success_200(body, "text/html");
}

std::string HTTPHandler::get_mime_type(const std::string &path)
{
	size_t dot_pos = path.find_last_of('.');
	if (dot_pos == std::string::npos)
	{
		return "application/octet-stream";
	}

	std::string ext = path.substr(dot_pos + 1);

	if (ext == "html" || ext == "htm")
	{
		return "text/html";
	}
	if (ext == "css")
	{
		return "text/css";
	}
	if (ext == "js")
	{
		return "application/javascript";
	}
	if (ext == "json")
	{
		return "application/json";
	}
	if (ext == "xml")
	{
		return "application/xml";
	}
	if (ext == "txt")
	{
		return "text/plain";
	}
	if (ext == "jpg" || ext == "jpeg")
	{
		return "image/jpeg";
	}
	if (ext == "png")
	{
		return "image/png";
	}
	if (ext == "gif")
	{
		return "image/gif";
	}
	if (ext == "svg")
	{
		return "image/svg+xml";
	}
	if (ext == "ico")
	{
		return "image/x-icon";
	}
	if (ext == "pdf")
	{
		return "application/pdf";
	}
	if (ext == "zip")
	{
		return "application/zip";
	}
	if (ext == "mp4")
	{
		return "video/mp4";
	}
	if (ext == "mp3")
	{
		return "audio/mpeg";
	}

	return "application/octet-stream";
}

std::string HTTPHandler::url_decode(const std::string &value)
{
	std::ostringstream decoded;
	for (size_t i = 0; i < value.length(); ++i)
	{
		if (value[i] == '%' && i + 2 < value.length())
		{
			int				   hex_val = 0;
			std::istringstream iss(value.substr(i + 1, 2));
			if ((iss >> std::hex >> hex_val) != 0)
			{
				decoded << static_cast<char>(hex_val);
				i += 2;
			}
			else
			{
				decoded << value[i];
			}
		}
		else if (value[i] == '+')
		{
			decoded << ' ';
		}
		else
		{
			decoded << value[i];
		}
	}
	return decoded.str();
}

//////////////////////////////////////////////////////////////// CGI response //

// Parse raw CGI stdout into an HTTPResponse.
// CGI output format (RFC 3875 §6):
//   Header-field CRLF
//   ...
//   CRLF
//   body
// A "Status:" header line overrides the default 200 OK status.
HTTPResponse HTTPHandler::parse_cgi_response(const std::string	&raw,
											 const ServerConfig &server)
{
	// Split headers from body on first blank line (\r\n\r\n or \n\n)
	std::string header_section;
	std::string body;

	size_t		split = raw.find("\r\n\r\n");
	if (split != std::string::npos)
	{
		header_section = raw.substr(0, split);
		body		   = raw.substr(split + 4);
	}
	else
	{
		split = raw.find("\n\n");
		if (split != std::string::npos)
		{
			header_section = raw.substr(0, split);
			body		   = raw.substr(split + 2);
		}
		else
		{
			// No blank line separator — treat entire output as body
			dprint("CGI: no header/body separator found in output");
			return HTTPResponse::error_500(get_error_page(500, server));
		}
	}

	// Parse individual header lines
	int			 status_code   = 200;
	std::string	 status_reason = "OK";

	HTTPResponse response;
	response.set_status(status_code, status_reason);

	size_t pos = 0;
	while (pos < header_section.size())
	{
		size_t eol = header_section.find('\n', pos);
		if (eol == std::string::npos)
		{
			eol = header_section.size();
		}
		std::string line = header_section.substr(pos, eol - pos);
		// Strip trailing \r
		if (!line.empty() && line[line.size() - 1] == '\r')
		{
			line.erase(line.size() - 1);
		}
		pos = eol + 1;

		if (line.empty())
		{
			continue;
		}

		size_t colon = line.find(':');
		if (colon == std::string::npos)
		{
			continue;
		}

		std::string key	  = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		// Trim leading spaces from value
		size_t		value_start = value.find_first_not_of(" \t");
		if (value_start != std::string::npos)
		{
			value = value.substr(value_start);
		}
		else
		{
			value = "";
		}

		// CGI "Status:" header sets the HTTP response status
		if (key == "Status")
		{
			size_t sp = value.find(' ');
			if (sp != std::string::npos)
			{
				status_code	  = std::atoi(value.substr(0, sp).c_str());
				status_reason = value.substr(sp + 1);
			}
			else
			{
				status_code = std::atoi(value.c_str());
			}
			response.set_status(status_code, status_reason);
		}
		else
		{
			response.set_header(key, value);
		}
	}

	response.set_body(body);

	dprint("CGI: parsed response: " << status_code << " " << status_reason
									<< ", body " << body.size() << " bytes");
	return response;
}

// Parse CGI response headers only (no body assignment).
// Used for streaming: the body is sent separately as it arrives.
HTTPResponse
HTTPHandler::parse_cgi_headers(const std::string  &cgi_header_section,
							   const ServerConfig &server)
{
	int			 status_code   = 200;
	std::string	 status_reason = "OK";

	HTTPResponse response;
	response.set_status(status_code, status_reason);

	size_t pos = 0;
	while (pos < cgi_header_section.size())
	{
		size_t eol = cgi_header_section.find('\n', pos);
		if (eol == std::string::npos)
			eol = cgi_header_section.size();
		std::string line = cgi_header_section.substr(pos, eol - pos);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		pos = eol + 1;

		if (line.empty())
			continue;

		size_t colon = line.find(':');
		if (colon == std::string::npos)
			continue;

		std::string key	  = line.substr(0, colon);
		std::string value = line.substr(colon + 1);
		size_t		vs	  = value.find_first_not_of(" \t");
		if (vs != std::string::npos)
			value = value.substr(vs);
		else
			value = "";

		if (key == "Status")
		{
			size_t sp = value.find(' ');
			if (sp != std::string::npos)
			{
				status_code	  = std::atoi(value.substr(0, sp).c_str());
				status_reason = value.substr(sp + 1);
			}
			else
			{
				status_code = std::atoi(value.c_str());
			}
			response.set_status(status_code, status_reason);
		}
		else
		{
			response.set_header(key, value);
		}
	}

	(void) server;
	return response;
}

///////////////////////////////////////////////////// Canonical Orthodox Form //

HTTPHandler::HTTPHandler()
{
}

HTTPHandler::HTTPHandler(const HTTPHandler &other)
{
	(void) other;
}

HTTPHandler &HTTPHandler::operator=(const HTTPHandler &other)
{
	(void) other;
	return *this;
}

HTTPHandler::~HTTPHandler()
{
}
