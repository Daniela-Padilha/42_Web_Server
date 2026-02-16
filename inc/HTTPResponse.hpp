#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <map>
#include <sstream>
#include <string>

class HTTPResponse
{
  private:
	std::string						   status_line_;
	std::map<std::string, std::string> headers_;
	std::string						   body_;

	std::string						   build_response() const;

  public:
	///////////////////////////////////////////////// Canonical Orthodox Form //
	HTTPResponse();
	HTTPResponse(const HTTPResponse &other);
	HTTPResponse &operator=(const HTTPResponse &other);
	~HTTPResponse();

	///////////////////////////////////////////////////////////////// Setters //
	void		set_status(int status_code, const std::string &reason_phrase);
	void		set_header(const std::string &key, const std::string &value);
	void		set_body(const std::string &body);

	///////////////////////////////////////////////////////////////// Getters //
	std::string to_string() const;

	////////////////////////////////////////////////////////////// Code pages //
	static HTTPResponse error_400();
	static HTTPResponse error_404();
	static HTTPResponse error_405();
	static HTTPResponse error_500();
	static HTTPResponse success_200(const std::string &body,
									const std::string &content_type);
	static HTTPResponse success_201(const std::string &body,
									const std::string &content_type);
	static HTTPResponse success_204();
};

#endif
