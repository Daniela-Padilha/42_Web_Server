#include "../inc/HTTPHandler.hpp"

static const std::string   STATIC_FILES_DIR = "files";
static const std::string   UPLOADS_DIR		= "uploads";

std::map<int, std::string> HTTPHandler::error_pages_;

void HTTPHandler::set_error_pages(const std::map<int, std::string> &pages)
{
	error_pages_ = pages;
}

std::string HTTPHandler::get_error_page(int status_code)
{
	std::map<int, std::string>::const_iterator it
		= error_pages_.find(status_code);
	if (it != error_pages_.end())
	{
		return it->second;
	}
	return "";
}

HTTPResponse HTTPHandler::handle_request(const HTTPRequest &request)
{
	const std::string &method = request.get_method();

	dprint("HTTPHandler: Handling " << method << " request for "
									<< request.get_request_target());

	if (method == "GET")
	{
		return handle_get(request);
	}
	if (method == "POST")
	{
		return handle_post(request);
	}
	if (method == "DELETE")
	{
		return handle_delete(request);
	}

	return HTTPResponse::error_405(get_error_page(405));
}

HTTPResponse HTTPHandler::handle_get(const HTTPRequest &request)
{
	std::string request_target = request.get_request_target();

	if (request_target == "/")
	{
		request_target = "/index.html";
	}

	std::string file_path = STATIC_FILES_DIR + request_target;

	dprint("HTTPHandler: GET file_path = " << file_path);

	struct stat file_stat;
	if (stat(file_path.c_str(), &file_stat) < 0)
	{
		dprint("HTTPHandler: File not found: " << file_path);
		return HTTPResponse::error_404(get_error_page(404));
	}

	if (S_ISDIR(file_stat.st_mode))
	{
		std::string index_path = file_path + "/index.html";
		if (stat(index_path.c_str(), &file_stat) == 0)
		{
			file_path = index_path;
		}
		else
		{
			return HTTPResponse::error_404(get_error_page(404));
		}
	}

	std::ifstream file(file_path.c_str(), std::ios::binary);
	if (!file)
	{
		dprint("HTTPHandler: Cannot open file: " << file_path);
		return HTTPResponse::error_500(get_error_page(500));
	}

	std::string body((std::istreambuf_iterator<char>(file)),
					 std::istreambuf_iterator<char>());
	file.close();

	std::string mime_type = get_mime_type(file_path);

	dprint("HTTPHandler: Serving file: " << file_path << " (" << body.size()
										 << " bytes, " << mime_type << ")");

	return HTTPResponse::success_200(body, mime_type);
}

HTTPResponse HTTPHandler::handle_post(const HTTPRequest &request)
{
	const std::string &content_type = request.get_header_value("content-type");
	const std::string &body			= request.get_body();

	dprint("HTTPHandler: POST content-type = " << content_type);

	if (content_type.find("multipart/form-data") != std::string::npos)
	{
		size_t boundary_pos = content_type.find("boundary=");
		if (boundary_pos != std::string::npos)
		{
			std::string boundary = content_type.substr(boundary_pos + 9);
			boundary			 = "--" + boundary;

			size_t filename_pos = body.find("filename=\"");
			if (filename_pos != std::string::npos)
			{
				size_t		filename_start = filename_pos + 10;
				size_t		filename_end   = body.find('\"', filename_start);
				std::string filename
					= body.substr(filename_start,
								  filename_end - filename_start);

				size_t data_start = body.find("\r\n\r\n", filename_end);
				if (data_start != std::string::npos)
				{
					data_start += 4;

					size_t data_end
						= body.find("\r\n--" + boundary.substr(2), data_start);
					if (data_end == std::string::npos)
					{
						data_end = body.length();
					}

					std::string file_data
						= body.substr(data_start, data_end - data_start);

					std::string	  upload_path = UPLOADS_DIR + "/" + filename;
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

		std::string	  upload_path = UPLOADS_DIR + "/form_data.txt";
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

HTTPResponse HTTPHandler::handle_delete(const HTTPRequest &request)
{
	const std::string &request_target = request.get_request_target();

	if (request_target == "/")
	{
		return HTTPResponse::error_400(get_error_page(400));
	}

	std::string file_path = STATIC_FILES_DIR + request_target;

	dprint("HTTPHandler: DELETE file_path = " << file_path);

	struct stat file_stat;
	if (stat(file_path.c_str(), &file_stat) < 0)
	{
		dprint("HTTPHandler: File not found for deletion: " << file_path);
		return HTTPResponse::error_404(get_error_page(404));
	}

	if (S_ISDIR(file_stat.st_mode))
	{
		return HTTPResponse::error_400(get_error_page(400));
	}

	if (unlink(file_path.c_str()) < 0)
	{
		dprint("HTTPHandler: Failed to delete file: " << strerror(errno));
		return HTTPResponse::error_500(get_error_page(500));
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

///////////////////////////////////////////////////// Canonical Orthodox Form //

HTTPHandler::HTTPHandler()
{
}

HTTPHandler::HTTPHandler(const HTTPHandler &other)
{
	(void) other; // Nothing to copy
}

HTTPHandler &HTTPHandler::operator=(const HTTPHandler &other)
{
	(void) other; // Nothing to assign
	return *this;
}

HTTPHandler::~HTTPHandler()
{
}
