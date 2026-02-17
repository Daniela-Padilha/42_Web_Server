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
				std::string ten = get_header_value("Transfer-Encoding");
				for (size_t i = 0; i < ten.length(); ++i)
				{
					ten[i] = std::tolower(ten[i]);
				}
				is_chunked_ = (ten.find("chunked") != std::string::npos);

				state_ = PARSSING_BODY_;
			}
			else
			{
				state_ = PARSSING_COMPLETE_;
			}
			dprint("HTTPRequest: Headers parsed successfully");
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

	// Transfer-Encoding: chunked takes precedence over Content-Length (RFC
	// 7230)
	iterator = headers_.find("transfer-encoding");
	if (iterator != headers_.end())
	{
		std::string value = iterator->second;
		for (size_t i = 0; i < value.length(); ++i)
		{
			value[i] = std::tolower(value[i]);
		}
		if (value.find("chunked") != std::string::npos)
		{
			return true;
		}
	}

	iterator = headers_.find("content-length");
	return iterator != headers_.end();
}

bool HTTPRequest::parse_body()
{
	if (is_chunked_)
	{
		return parse_chunked_body();
	}

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

	return true;
}

///////////////////////////////////////////// Chunked transfer encoding parser
/////
// State machine:
//   CHUNK_SIZE_      -> read hex size line (terminated by CRLF), may have
//                       chunk-ext after ';' which we ignore
//   CHUNK_DATA_      -> read exactly chunk_remaining_ bytes of data
//   CHUNK_DATA_CRLF_ -> consume the CRLF after chunk data
//   CHUNK_TRAILERS_  -> after the final 0-size chunk, consume trailer
//                       headers until an empty line (CRLF)

bool HTTPRequest::parse_chunked_body()
{
	while (true)
	{
		if (chunked_state_ == CHUNK_SIZE_)
		{
			size_t pos = buffer_.find("\r\n");
			if (pos == std::string::npos)
			{
				dprint("HTTPRequest: parse_chunked_body: "
					   "Waiting for chunk size line");
				return false;
			}

			std::string size_line = buffer_.substr(0, pos);
			buffer_.erase(0, pos + 2);

			// Strip optional chunk extensions after ';'
			size_t semi = size_line.find(';');
			if (semi != std::string::npos)
			{
				size_line = size_line.substr(0, semi);
			}

			// Trim whitespace
			size_t start = size_line.find_first_not_of(" \t");
			size_t end	 = size_line.find_last_not_of(" \t");
			if (start == std::string::npos)
			{
				eprint("HTTPRequest: parse_chunked_body: "
					   "Empty chunk size line");
				state_ = PARSSING_ERROR_;
				return false;
			}
			size_line = size_line.substr(start, end - start + 1);

			// Validate hex digits
			for (size_t i = 0; i < size_line.length(); ++i)
			{
				char chr = size_line[i];
				if ((chr < '0' || chr > '9') && (chr < 'a' || chr > 'f')
					&& (chr < 'A' || chr > 'F'))
				{
					eprint("HTTPRequest: parse_chunked_body: "
						   "Invalid hex in chunk size: "
						   + size_line);
					state_ = PARSSING_ERROR_;
					return false;
				}
			}

			// Parse hex size
			chunk_remaining_ = 0;
			for (size_t i = 0; i < size_line.length(); ++i)
			{
				char   chr	 = size_line[i];
				size_t digit = 0;
				if (chr >= '0' && chr <= '9')
				{
					digit = chr - '0';
				}
				else if (chr >= 'a' && chr <= 'f')
				{
					digit = 10 + (chr - 'a');
				}
				else if (chr >= 'A' && chr <= 'F')
				{
					digit = 10 + (chr - 'A');
				}
				chunk_remaining_ = (chunk_remaining_ * 16) + digit;
			}

			dprint("HTTPRequest: parse_chunked_body: "
				   "chunk size = "
				   << chunk_remaining_);

			if (chunk_remaining_ == 0)
			{
				chunked_state_ = CHUNK_TRAILERS_;
			}
			else
			{
				chunked_state_ = CHUNK_DATA_;
			}
		}
		else if (chunked_state_ == CHUNK_DATA_)
		{
			if (buffer_.length() >= chunk_remaining_)
			{
				body_ += buffer_.substr(0, chunk_remaining_);
				buffer_.erase(0, chunk_remaining_);
				chunk_remaining_ = 0;
				chunked_state_	 = CHUNK_DATA_CRLF_;
			}
			else
			{
				chunk_remaining_ -= buffer_.length();
				body_ += buffer_;
				buffer_.clear();
				dprint("HTTPRequest: parse_chunked_body: "
					   "Waiting for more chunk data, remaining = "
					   << chunk_remaining_);
				return false;
			}
		}
		else if (chunked_state_ == CHUNK_DATA_CRLF_)
		{
			if (buffer_.length() < 2)
			{
				dprint("HTTPRequest: parse_chunked_body: "
					   "Waiting for CRLF after chunk data");
				return false;
			}

			if (buffer_[0] != '\r' || buffer_[1] != '\n')
			{
				eprint("HTTPRequest: parse_chunked_body: "
					   "Missing CRLF after chunk data");
				state_ = PARSSING_ERROR_;
				return false;
			}

			buffer_.erase(0, 2);
			chunked_state_ = CHUNK_SIZE_;
		}
		else if (chunked_state_ == CHUNK_TRAILERS_)
		{
			size_t pos = buffer_.find("\r\n");
			if (pos == std::string::npos)
			{
				dprint("HTTPRequest: parse_chunked_body: "
					   "Waiting for trailer/final CRLF");
				return false;
			}

			std::string line = buffer_.substr(0, pos);
			buffer_.erase(0, pos + 2);

			if (line.empty())
			{
				dprint("HTTPRequest: parse_chunked_body: "
					   "Chunked body complete, total body size = "
					   << body_.length());
				return true;
			}

			dprint("HTTPRequest: parse_chunked_body: "
				   "Ignoring trailer: "
				   + line);
		}
	}
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
	state_			 = PARSSING_REQUEST_LINE_;
	chunked_state_	 = CHUNK_SIZE_;
	chunk_remaining_ = 0;
	is_chunked_		 = false;
	method_.clear();
	request_target_.clear();
	protocol_.clear();
	headers_.clear();
	body_.clear();
}

///////////////////////////////////////////////////// Canonical Orthodox Form //
HTTPRequest::HTTPRequest() :
	state_(PARSSING_REQUEST_LINE_),
	chunked_state_(CHUNK_SIZE_),
	chunk_remaining_(0),
	is_chunked_(false)
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

	buffer_			 = other.buffer_;
	state_			 = other.state_;
	chunked_state_	 = other.chunked_state_;
	chunk_remaining_ = other.chunk_remaining_;
	is_chunked_		 = other.is_chunked_;
	method_			 = other.method_;
	request_target_	 = other.request_target_;
	protocol_		 = other.protocol_;
	headers_		 = other.headers_;
	body_			 = other.body_;

	return (*this);
}

HTTPRequest::~HTTPRequest()
{
	dprint("HTTPRequest: Default Destructor");
}
