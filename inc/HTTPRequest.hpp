#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <cctype>
#include <cstdlib>
#include <map>
#include <string>

#include "../inc/utils_print.hpp"

class HTTPRequest
{
  private:
	std::string buffer_;

	/////////////////////////////////////////////////////////// Request state //
	enum ParseState
	{
		PARSSING_REQUEST_LINE_,
		PARSSING_HEADERS_,
		PARSSING_BODY_,
		PARSSING_COMPLETE_,
		PARSSING_ERROR_
	};

	ParseState state_;

	///////////////////////////////////////// Chunked transfer encoding state //
	enum ChunkedState
	{
		CHUNK_SIZE_,
		CHUNK_DATA_,
		CHUNK_DATA_CRLF_,
		CHUNK_TRAILERS_
	};

	ChunkedState					   chunked_state_;
	size_t							   chunk_remaining_;
	bool							   is_chunked_;

	/////////////////////////////////////////////////////////// Request parts //
	std::string						   method_;
	std::string						   request_target_;
	std::string						   protocol_;

	std::map<std::string, std::string> headers_;

	std::string						   body_;

	///////////////////////////////////////////////////////////// Parse utils //
	bool							   parse_request_line();
	bool							   parse_headers();
	bool							   has_body() const;
	bool							   parse_body();
	bool							   parse_chunked_body();

  public:
	///////////////////////////////////////////////// Canonical Orthodox Form //
	HTTPRequest();
	HTTPRequest(const HTTPRequest &other);
	HTTPRequest &operator=(const HTTPRequest &other);
	~HTTPRequest();

	///////////////////////////////////////////////////////////////// Getters //
	const std::string &get_method() const;
	const std::string &get_request_target() const;
	const std::string &get_protocol() const;

	std::string		   get_header_value(const std::string &key) const;

	const std::string &get_body() const;

	/////////////////////////////////////////////////////////////////// Utils //
	bool			   parse(const std::string &buffer);
	bool			   is_error() const;
	void			   reset();
};

#endif
