#include "../inc/HTTPRequest.hpp"

/////////////////////////////////////////////////////////////////////// parse //
bool HTTPRequest::parse(const std::string &buffer)
{
	// dprint("HTTPRequest: parse: previous buffer  <" << this->buffer_ << ">");
	// dprint("HTTPRequest: parse: append to buffer <" << buffer << ">");

	buffer_ += buffer;

	if (DEBUG)
	{
		dprint("HTTPRequest: parse: current buffer bellow:");
		std::cout << BG_BOLD_BLUE << buffer_ << RESET << '\n';
	}

	while (state_ != PARSSING_COMPLETE_ && state_ != PARSSING_ERROR_)
	{
		if (state_ == PARSSING_REQUEST_LINE_)
		{
			if (!parse_request_line())
			{
				dprint("HTTPRequest: parse: Incomplete request line");
				return false;
			}
			state_ = PARSSING_HEADERS_;
			dprint("HTTPRequest: parse: Request line parsed successfully");
		}
		else if (state_ == PARSSING_HEADERS_)
		{
			if (!parse_headers())
			{
				dprint("HTTPRequest: parse: Incomplete headers");
				return false;
			}
			if (has_body())
			{
				state_ = PARSSING_BODY_;
			}
			else
			{
				state_ = PARSSING_COMPLETE_;
			}
			dprint("Headers parsed successfully");
		}
		else if (state_ == PARSSING_BODY_)
		{
			if (!parse_body())
			{
				dprint("HTTPRequest: parse: Incomplete body");
				return false;
			}
			state_ = PARSSING_COMPLETE_;
			dprint("Body parsed successfully");
		}
	}

	return (state_ == PARSSING_COMPLETE_);
}

///////////////////////////////////////////////////////////////// parse utils //
bool HTTPRequest::parse_request_line()
{
	size_t pos = buffer_.find("\r\n");
	if (pos == std::string::npos)
	{
		dprint("HTTPRequest: parse_request: Line not complete yet");
		return false;
	}

	std::string line = buffer_.substr(0, pos);
	buffer_.erase(0, pos + 2);

	dprint("HTTPRequest: parse_request: Parsing request line: " + line);
	size_t firstSpace = line.find(' ');
	if (firstSpace == std::string::npos)
	{
		eprint("HTTPRequest: parse_request: "
			   "Invalid request line: no first space");
		state_ = PARSSING_ERROR_;
		return false;
	}

	size_t secondSpace = line.find(' ', firstSpace + 1);
	if (secondSpace == std::string::npos)
	{
		eprint("HTTPRequest: parse_request: "
			   "Invalid request line: no second space");
		state_ = PARSSING_ERROR_;
		return false;
	}

	method_ = line.substr(0, firstSpace);
	dprint("HTTPRequest: parse_request: method: " + method_);
	request_target_ = line.substr(firstSpace + 1, secondSpace - firstSpace - 1);
	dprint("HTTPRequest: parse_request: request target: " + request_target_);
	protocol_ = line.substr(secondSpace + 1);
	dprint("HTTPRequest: parse_request: protocol: " + protocol_);

	if (method_.empty() || request_target_.empty() || protocol_.empty())
	{
		eprint("HTTPRequest: parse_request: "
			   "Invalid request line: empty component");
		state_ = PARSSING_ERROR_;
		return false;
	}

	return true;
}

bool HTTPRequest::parse_headers()
{
	while (true)
	{
		size_t pos = buffer_.find("\r\n");
		if (pos == std::string::npos)
		{
			dprint("HTTPRequest: parse_headers: Line not complete yet");
			return false;
		}

		std::string line = buffer_.substr(0, pos);
		buffer_.erase(0, pos + 2);

		if (line.empty())
		{
			dprint("HTTPRequest: parse_headers: End of headers");
			return true;
		}

		size_t colon = line.find(':');
		if (colon == std::string::npos)
		{
			eprint("HTTPRequest: parse_headers: "
				   "Invalid header line: no colon");
			state_ = PARSSING_ERROR_;
			return false;
		}

		std::string key	  = line.substr(0, colon);
		std::string value = line.substr(colon + 1);

		for (size_t i = 0; i < key.length(); ++i)
		{
			key[i] = std::tolower(key[i]);
		}

		size_t valueStart = value.find_first_not_of(" \t");
		size_t valueEnd	  = value.find_last_not_of(" \t");
		if (valueStart != std::string::npos && valueEnd != std::string::npos)
		{
			value = value.substr(valueStart, valueEnd - valueStart + 1);
		}
		else
		{
			value.clear();
		}

		headers_[key] = value;
		dprint("Parsed header: " + key + " = " + value);
	}
}

bool HTTPRequest::has_body() const
{
	std::map<std::string, std::string>::const_iterator iterator;
	iterator = headers_.find("content-length");
	if (iterator != headers_.end())
	{
		return true;
	}

	// Not on project scope
	// iterator = headers_.find("transfer-encoding");
	// if (iterator != headers_.end() && iterator->second == "chunked")
	// {
	// 	return true;
	// }

	return false;
}

bool HTTPRequest::parse_body()
{
	std::string content_length = get_header_value("Content-Length");
	if (!content_length.empty())
	{
		size_t len		= std::atol(content_length.c_str());
		size_t body_len = body_.length();

		if (len > body_len)
		{
			size_t needed = len - body_len;

			if (buffer_.length() >= needed)
			{
				body_ += buffer_.substr(0, needed);
				buffer_.erase(0, needed);
				return true;
			}

			body_ += buffer_;
			buffer_.clear();
			return false;
		}
		return true;
	}

	if (get_header_value("Transfer-Encoding").find("chunked")
		!= std::string::npos)
	{
		dprint("HTTPRequest: parse_body: "
			   "Chunked transfer encoding not implemented");
		return true;
	}

	return true;
}

///////////////////////////////////////////////////////////////////// getters //
const std::string &HTTPRequest::get_method() const
{
	return method_;
}

const std::string &HTTPRequest::get_request_target() const
{
	return request_target_;
}

const std::string &HTTPRequest::get_protocol() const
{
	return protocol_;
}

std::string HTTPRequest::get_header_value(const std::string &key) const
{
	std::string lower_key = key;
	for (size_t i = 0; i < lower_key.length(); ++i)
	{
		lower_key[i] = std::tolower(lower_key[i]);
	}

	std::map<std::string, std::string>::const_iterator iterator
		= headers_.find(lower_key);
	if (iterator != headers_.end())
	{
		return iterator->second;
	}
	return "";
}

const std::string &HTTPRequest::get_body() const
{
	return body_;
}

//////////////////////////////////////////////////////////////////// checking //
bool HTTPRequest::is_error() const
{
	return state_ == PARSSING_ERROR_;
}

//////////////////////////////////////////////////////////////////// clean up //
void HTTPRequest::reset()
{
	buffer_.clear();
	state_ = PARSSING_REQUEST_LINE_;
	method_.clear();
	request_target_.clear();
	protocol_.clear();
	headers_.clear();
	body_.clear();
}

///////////////////////////////////////////////////// Canonical Orthodox Form //
HTTPRequest::HTTPRequest() : state_(PARSSING_REQUEST_LINE_)
{
	dprint("HTTPRequest: Default Constructor");
}

HTTPRequest::HTTPRequest(const HTTPRequest &other)
{
	dprint("HTTPRequest: Copy Constructor");
	*this = other;
}

HTTPRequest &HTTPRequest::operator=(const HTTPRequest &other)
{
	dprint("HTTPRequest: Copy Operator Constructor");
	if (this == &other)
	{
		return (*this);
	}

	buffer_			= other.buffer_;
	state_			= other.state_;
	method_			= other.method_;
	request_target_ = other.request_target_;
	protocol_		= other.protocol_;
	headers_		= other.headers_;
	body_			= other.body_;

	return (*this);
}

HTTPRequest::~HTTPRequest()
{
	dprint("HTTPRequest: Default Destructor");
}
