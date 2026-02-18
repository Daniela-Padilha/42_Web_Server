#include "../inc/HTTPResponse.hpp"
#include "../inc/utils_print.hpp"

std::string HTTPResponse::load_error_page(const std::string &page_path,
										  const std::string &fallback)
{
	if (page_path.empty())
	{
		return fallback;
	}
	std::ifstream file(page_path.c_str(), std::ios::binary);
	if (!file.is_open())
	{
		dprint("HTTPResponse: Cannot open error page: " << page_path);
		return fallback;
	}
	std::string content((std::istreambuf_iterator<char>(file)),
						std::istreambuf_iterator<char>());
	file.close();
	return content;
}

HTTPResponse::HTTPResponse()
{
}

HTTPResponse::HTTPResponse(const HTTPResponse &other) :
	status_line_(other.status_line_),
	headers_(other.headers_),
	body_(other.body_)
{
}

HTTPResponse &HTTPResponse::operator=(const HTTPResponse &other)
{
	if (this != &other)
	{
		status_line_ = other.status_line_;
		headers_	 = other.headers_;
		body_		 = other.body_;
	}
	return *this;
}

HTTPResponse::~HTTPResponse()
{
}

void HTTPResponse::set_status(int status_code, const std::string &reason_phrase)
{
	std::ostringstream oss;
	oss << "HTTP/1.1 " << status_code << " " << reason_phrase;
	status_line_ = oss.str();
}

void HTTPResponse::set_header(const std::string &key, const std::string &value)
{
	headers_[key] = value;
}

void HTTPResponse::set_body(const std::string &body)
{
	body_ = body;
	std::ostringstream oss;
	oss << body.length();
	set_header("Content-Length", oss.str());
}

std::string HTTPResponse::build_response() const
{
	std::ostringstream oss;

	oss << status_line_ << "\r\n";

	for (std::map<std::string, std::string>::const_iterator it
		 = headers_.begin();
		 it != headers_.end();
		 ++it)
	{
		oss << it->first << ": " << it->second << "\r\n";
	}

	oss << "\r\n";

	oss << body_;

	return oss.str();
}

std::string HTTPResponse::to_string() const
{
	return build_response();
}

HTTPResponse HTTPResponse::error_400(const std::string &page_path)
{
	HTTPResponse res;
	res.set_status(400, "Bad Request");
	res.set_body(load_error_page(page_path, "400 Bad Request"));
	res.set_header("Content-Type", "text/html");
	return res;
}

HTTPResponse HTTPResponse::error_404(const std::string &page_path)
{
	HTTPResponse res;
	res.set_status(404, "Not Found");
	res.set_body(load_error_page(page_path, "404 Not Found"));
	res.set_header("Content-Type", "text/html");
	return res;
}

HTTPResponse HTTPResponse::error_405(const std::string &page_path)
{
	HTTPResponse res;
	res.set_status(405, "Method Not Allowed");
	res.set_body(load_error_page(page_path, "405 Method Not Allowed"));
	res.set_header("Content-Type", "text/html");
	return res;
}

HTTPResponse HTTPResponse::error_500(const std::string &page_path)
{
	HTTPResponse res;
	res.set_status(500, "Internal Server Error");
	res.set_body(load_error_page(page_path, "500 Internal Server Error"));
	res.set_header("Content-Type", "text/html");
	return res;
}

HTTPResponse HTTPResponse::success_200(const std::string &body,
									   const std::string &content_type)
{
	HTTPResponse res;
	res.set_status(200, "OK");
	res.set_body(body);
	res.set_header("Content-Type", content_type);
	return res;
}

HTTPResponse HTTPResponse::success_201(const std::string &body,
									   const std::string &content_type)
{
	HTTPResponse res;
	res.set_status(201, "Created");
	res.set_body(body);
	res.set_header("Content-Type", content_type);
	return res;
}

HTTPResponse HTTPResponse::success_204()
{
	HTTPResponse res;
	res.set_status(204, "No Content");
	return res;
}
