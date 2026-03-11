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

	///////////////////////////////////////////////////////////// Body limits //
	size_t							   max_body_size_;
	bool							   body_too_large_;

	///////////////////////////////////////////////////////////// Parse utils //
	bool							   parse_request_line();
	bool							   parse_headers();
	bool							   has_body() const;
	bool							   parse_body();
	bool							   parse_chunked_body();
	bool							   parse_chunk_size();
	bool							   parse_chunk_data();
	bool							   parse_chunk_data_crlf();
	bool							   parse_chunk_trailers();
	static bool parse_hex_string(const std::string &str, size_t &out);
	static void str_to_lower(std::string &str);

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
	bool			   is_body_too_large() const;
	void			   set_max_body_size(size_t size);
	void			   reset();
};

#endif
