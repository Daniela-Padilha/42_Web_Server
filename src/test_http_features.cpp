#include "../inc/tests.hpp"

void test_http_features()
{
	dprint("Running HTTP Features Tests\n");

	test_autoindex_generation();
	test_runtime_redirection();
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

bool test_autoindex_generation()
{
	bool		 all_passed = true;
	RouteConfig	 route;
	ServerConfig server;
	HTTPRequest	 req;

	route.autoindex = true;
	route.root		= "YoupiBanane";
	route.path		= "/directory/";

	const char *raw_req = "GET /directory/ HTTP/1.1\r\nHost: localhost\r\n\r\n";
	req.parse(raw_req);

	HTTPResponse res = HTTPHandler::handle_request(req, &route, server);
	std::string	 raw = res.to_string();

	if (raw.find("HTTP/1.1 200 OK") == std::string::npos)
	{
		std::cout << "test failed: status should be 200 OK\n";
		all_passed = false;
	}

	all_passed &= check_contains(raw,
								 "Index of /directory/",
								 "test_autoindex_generation");
	all_passed &= check_contains(raw,
								 "<a href=\"youpi.bla\">youpi.bla</a>",
								 "test_autoindex_generation");
	all_passed &= check_contains(raw,
								 "<a href=\"nop/\">nop/</a>",
								 "test_autoindex_generation");
	all_passed &= check_contains(raw,
								 "Content-Type: text/html",
								 "test_autoindex_generation");

	if (all_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_autoindex_generation passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_autoindex_generation failed\n";
	}
	return all_passed;
}

bool test_runtime_redirection()
{
	bool		 all_passed = true;
	RouteConfig	 route;
	ServerConfig server;
	HTTPRequest	 req;

	// Test 301
	route.redirect_code = 301;
	route.redirect_url	= "/new-location";

	HTTPResponse res = HTTPHandler::handle_request(req, &route, server);
	std::string	 raw = res.to_string();

	if (raw.find("HTTP/1.1 301") == std::string::npos)
	{
		std::cout << "test failed: status should be 301\n";
		all_passed = false;
	}

	all_passed &= check_contains(raw,
								 "Location: /new-location",
								 "test_runtime_redirection");

	// Test 302
	route.redirect_code = 302;
	route.redirect_url	= "/temp-location";
	res					= HTTPHandler::handle_request(req, &route, server);
	raw					= res.to_string();

	if (raw.find("HTTP/1.1 302") == std::string::npos)
	{
		std::cout << "test failed: status should be 302\n";
		all_passed = false;
	}

	all_passed &= check_contains(raw,
								 "Location: /temp-location",
								 "test_runtime_redirection");

	if (all_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_runtime_redirection passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_runtime_redirection failed\n";
	}
	return all_passed;
}
