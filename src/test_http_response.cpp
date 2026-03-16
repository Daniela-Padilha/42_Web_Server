#include "../inc/tests.hpp"

void test_http_response()
{
	dprint("Running HTTP Response Tests\n");

	test_response_error_400();
	test_response_error_403();
	test_response_error_404();
	test_response_error_405();
	test_response_error_413();
	test_response_error_500();
	test_response_redirect_301();
	test_response_redirect_302();
	test_response_error_page_from_file();
}

static bool check_contains(const std::string &haystack,
						   const std::string &needle,
						   const std::string &label)
{
	if (haystack.find(needle) == std::string::npos)
	{
		std::cout << "test failed: expected " << label << " to contain '"
				  << needle << "'\n";
		return false;
	}
	return true;
}

static bool test_error_response(const std::string &name,
								HTTPResponse (*factory)(const std::string &),
								const std::string &status_line,
								const std::string &fallback_body)
{
	bool		 all_tests_passed = true;
	HTTPResponse res			  = factory("");
	std::string	 raw			  = res.to_string();

	if (!check_contains(raw, status_line, name))
	{
		all_tests_passed = false;
	}
	if (!check_contains(raw, "Content-Type: text/html", name))
	{
		all_tests_passed = false;
	}
	if (!check_contains(raw, "Content-Length:", name))
	{
		all_tests_passed = false;
	}
	if (!check_contains(raw, fallback_body, name))
	{
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET << " " << name << " passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET << " " << name << " failed\n";
	}
	return all_tests_passed;
}

bool test_response_error_400()
{
	return test_error_response("test_response_error_400",
							   &HTTPResponse::error_400,
							   "HTTP/1.1 400 Bad Request",
							   "400 Bad Request");
}

bool test_response_error_403()
{
	return test_error_response("test_response_error_403",
							   &HTTPResponse::error_403,
							   "HTTP/1.1 403 Forbidden",
							   "403 Forbidden");
}

bool test_response_error_404()
{
	return test_error_response("test_response_error_404",
							   &HTTPResponse::error_404,
							   "HTTP/1.1 404 Not Found",
							   "404 Not Found");
}

bool test_response_error_405()
{
	return test_error_response("test_response_error_405",
							   &HTTPResponse::error_405,
							   "HTTP/1.1 405 Method Not Allowed",
							   "405 Method Not Allowed");
}

bool test_response_error_413()
{
	return test_error_response("test_response_error_413",
							   &HTTPResponse::error_413,
							   "HTTP/1.1 413 Payload Too Large",
							   "413 Payload Too Large");
}

bool test_response_error_500()
{
	return test_error_response("test_response_error_500",
							   &HTTPResponse::error_500,
							   "HTTP/1.1 500 Internal Server Error",
							   "500 Internal Server Error");
}

bool test_response_redirect_301()
{
	bool		 all_tests_passed = true;
	HTTPResponse res			  = HTTPResponse::redirect_301("/new-location");
	std::string	 raw			  = res.to_string();

	if (!check_contains(raw,
						"HTTP/1.1 301 Moved Permanently",
						"test_response_redirect_301"))
	{
		all_tests_passed = false;
	}
	if (!check_contains(raw,
						"Location: /new-location",
						"test_response_redirect_301"))
	{
		all_tests_passed = false;
	}
	if (!check_contains(raw,
						"Content-Type: text/html",
						"test_response_redirect_301"))
	{
		all_tests_passed = false;
	}
	if (!check_contains(raw, "Content-Length:", "test_response_redirect_301"))
	{
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_response_redirect_301 passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_response_redirect_301 failed\n";
	}
	return all_tests_passed;
}

bool test_response_redirect_302()
{
	bool		 all_tests_passed = true;
	HTTPResponse res			  = HTTPResponse::redirect_302("/temporary");
	std::string	 raw			  = res.to_string();

	if (!check_contains(raw,
						"HTTP/1.1 302 Found",
						"test_response_redirect_302"))
	{
		all_tests_passed = false;
	}
	if (!check_contains(raw,
						"Location: /temporary",
						"test_response_redirect_302"))
	{
		all_tests_passed = false;
	}
	if (!check_contains(raw,
						"Content-Type: text/html",
						"test_response_redirect_302"))
	{
		all_tests_passed = false;
	}
	if (!check_contains(raw, "Content-Length:", "test_response_redirect_302"))
	{
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_response_redirect_302 passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_response_redirect_302 failed\n";
	}
	return all_tests_passed;
}

bool test_response_error_page_from_file()
{
	bool		 all_tests_passed = true;
	HTTPResponse res = HTTPResponse::error_404("error_pages/404.html");
	std::string	 raw = res.to_string();

	if (!check_contains(raw,
						"HTTP/1.1 404 Not Found",
						"test_response_error_page_from_file"))
	{
		all_tests_passed = false;
	}
	if (!check_contains(raw,
						"404 Not Found",
						"test_response_error_page_from_file"))
	{
		all_tests_passed = false;
	}
	if (raw.find("404") == std::string::npos)
	{
		std::cout << "test failed: expected file content with "
				  << "404\n";
		all_tests_passed = false;
	}
	if (!check_contains(raw,
						"Content-Type: text/html",
						"test_response_error_page_from_file"))
	{
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_response_error_page_from_file passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_response_error_page_from_file failed\n";
	}
	return all_tests_passed;
}
