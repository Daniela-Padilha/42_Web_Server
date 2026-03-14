#include "../inc/HTTPRequest.hpp"

static const size_t CRLF_LEN = 2;

void				HTTPRequest::str_to_lower(std::string &str)
{
	for (size_t i = 0; i < str.length(); ++i)
	{
		str[i] = static_cast<char>(
			std::tolower(static_cast<unsigned char>(str[i])));
	}
}

/////////////////////////////////////////////////////////////////////// parse //
bool HTTPRequest::parse(const std::string &buffer)
{
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
				str_to_lower(ten);
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
	buffer_.erase(0, pos + CRLF_LEN);

	dprint("HTTPRequest: parse_request: Parsing request line: " + line);
	size_t first_space = line.find(' ');
	if (first_space == std::string::npos)
	{
		eprint("HTTPRequest: parse_request: "
			   "Invalid request line: no first space");
		state_ = PARSSING_ERROR_;
		return false;
	}

	size_t second_space = line.find(' ', first_space + 1);
	if (second_space == std::string::npos)
	{
		eprint("HTTPRequest: parse_request: "
			   "Invalid request line: no second space");
		state_ = PARSSING_ERROR_;
		return false;
	}

	method_ = line.substr(0, first_space);
	dprint("HTTPRequest: parse_request: method: " + method_);
	request_target_
		= line.substr(first_space + 1, second_space - first_space - 1);
	dprint("HTTPRequest: parse_request: request target: " + request_target_);
	protocol_ = line.substr(second_space + 1);
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
		buffer_.erase(0, pos + CRLF_LEN);

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

		str_to_lower(key);

		size_t value_start = value.find_first_not_of(" \t");
		size_t value_end   = value.find_last_not_of(" \t");
		if (value_start != std::string::npos && value_end != std::string::npos)
		{
			value = value.substr(value_start, value_end - value_start + 1);
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
		str_to_lower(value);
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
		size_t len = std::atol(content_length.c_str());

		if (max_body_size_ > 0 && len > max_body_size_)
		{
			eprint("HTTPRequest: Content-Length "
				   << len << " exceeds max body size " << max_body_size_);
			body_too_large_ = true;
			state_			= PARSSING_ERROR_;
			return false;
		}

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

//////////////////////////////////////////// Chunked transfer encoding parser //
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
		bool done = false;
		if (chunked_state_ == CHUNK_SIZE_)
		{
			done = parse_chunk_size();
		}
		else if (chunked_state_ == CHUNK_DATA_)
		{
			done = parse_chunk_data();
		}
		else if (chunked_state_ == CHUNK_DATA_CRLF_)
		{
			done = parse_chunk_data_crlf();
		}
		else if (chunked_state_ == CHUNK_TRAILERS_)
		{
			return parse_chunk_trailers();
		}
		if (!done)
		{
			return false;
		}
	}
}

bool HTTPRequest::parse_hex_string(const std::string &str, size_t &out)
{
	for (size_t i = 0; i < str.length(); ++i)
	{
		char chr = str[i];
		if ((chr < '0' || chr > '9') && (chr < 'a' || chr > 'f')
			&& (chr < 'A' || chr > 'F'))
		{
			return false;
		}
	}
	out = 0;
	for (size_t i = 0; i < str.length(); ++i)
	{
		char   chr	 = str[i];
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
		out = (out * 16) + digit;
	}
	return true;
}

bool HTTPRequest::parse_chunk_size()
{
	size_t pos = buffer_.find("\r\n");
	if (pos == std::string::npos)
	{
		dprint("HTTPRequest: parse_chunk_size: "
			   "Waiting for chunk size line");
		return false;
	}

	std::string size_line = buffer_.substr(0, pos);
	buffer_.erase(0, pos + CRLF_LEN);

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
		eprint("HTTPRequest: parse_chunk_size: "
			   "Empty chunk size line");
		state_ = PARSSING_ERROR_;
		return false;
	}
	size_line = size_line.substr(start, end - start + 1);

	if (!parse_hex_string(size_line, chunk_remaining_))
	{
		eprint("HTTPRequest: parse_chunk_size: "
			   "Invalid hex in chunk size: "
			   + size_line);
		state_ = PARSSING_ERROR_;
		return false;
	}

	dprint("HTTPRequest: parse_chunk_size: "
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
	return true;
}

bool HTTPRequest::parse_chunk_data()
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
		dprint("HTTPRequest: parse_chunk_data: "
			   "Waiting for more chunk data, remaining = "
			   << chunk_remaining_);
		return false;
	}
	if (max_body_size_ > 0 && body_.size() > max_body_size_)
	{
		eprint("HTTPRequest: Chunked body size "
			   << body_.size() << " exceeds max body size " << max_body_size_);
		body_too_large_ = true;
		state_			= PARSSING_ERROR_;
		return false;
	}
	return true;
}

bool HTTPRequest::parse_chunk_data_crlf()
{
	if (buffer_.length() < CRLF_LEN)
	{
		dprint("HTTPRequest: parse_chunk_data_crlf: "
			   "Waiting for CRLF after chunk data");
		return false;
	}

	if (buffer_[0] != '\r' || buffer_[1] != '\n')
	{
		eprint("HTTPRequest: parse_chunk_data_crlf: "
			   "Missing CRLF after chunk data");
		state_ = PARSSING_ERROR_;
		return false;
	}

	buffer_.erase(0, CRLF_LEN);
	chunked_state_ = CHUNK_SIZE_;
	return true;
}

bool HTTPRequest::parse_chunk_trailers()
{
	while (true)
	{
		size_t pos = buffer_.find("\r\n");
		if (pos == std::string::npos)
		{
			dprint("HTTPRequest: parse_chunk_trailers: "
				   "Waiting for trailer/final CRLF");
			return false;
		}

		std::string line = buffer_.substr(0, pos);
		buffer_.erase(0, pos + CRLF_LEN);

		if (line.empty())
		{
			dprint("HTTPRequest: parse_chunk_trailers: "
				   "Chunked body complete, total body size = "
				   << body_.length());
			return true;
		}

		dprint("HTTPRequest: parse_chunk_trailers: "
			   "Ignoring trailer: "
			   + line);
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
	str_to_lower(lower_key);

	std::map<std::string, std::string>::const_iterator iterator
		= headers_.find(lower_key);
	if (iterator != headers_.end())
	{
		return iterator->second;
	}
	return "";
}

const std::map<std::string, std::string> &HTTPRequest::get_headers() const
{
	return headers_;
}

const std::string &HTTPRequest::get_body() const
{
	return body_;
}

const std::string &HTTPRequest::get_internal_buffer() const
{
	return buffer_;
}

bool HTTPRequest::is_chunked() const
{
	return is_chunked_;
}

size_t HTTPRequest::get_chunk_remaining() const
{
	return chunk_remaining_;
}

int HTTPRequest::get_chunked_state() const
{
	return static_cast<int>(chunked_state_);
}

//////////////////////////////////////////////////////////////////// checking //
bool HTTPRequest::is_error() const
{
	return state_ == PARSSING_ERROR_;
}

bool HTTPRequest::is_body_too_large() const
{
	return body_too_large_;
}

bool HTTPRequest::is_headers_done() const
{
	return state_ == PARSSING_BODY_ || state_ == PARSSING_COMPLETE_;
}

void HTTPRequest::set_max_body_size(size_t size)
{
	max_body_size_ = size;
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
	max_body_size_	= 0;
	body_too_large_ = false;
}

///////////////////////////////////////////////////// Canonical Orthodox Form //
HTTPRequest::HTTPRequest() :
	state_(PARSSING_REQUEST_LINE_),
	chunked_state_(CHUNK_SIZE_),
	chunk_remaining_(0),
	is_chunked_(false),
	max_body_size_(0),
	body_too_large_(false)
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
	max_body_size_	 = other.max_body_size_;
	body_too_large_	 = other.body_too_large_;

	return (*this);
}

HTTPRequest::~HTTPRequest()
{
	dprint("HTTPRequest: Default Destructor");
}
