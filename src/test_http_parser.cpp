#include "../inc/tests.hpp"

void test_http_parser()
{
	dprint("Running HTTP Parser Tests\n");

	test_simple_get_request();
	test_packet_fragmentation();
	test_post_with_body();
	test_post_body_in_chunks();
	test_invalid_request_line();
	test_header_without_colon();
	test_multiple_headers();
	test_header_value_with_spaces();
	test_empty_uri();
	test_large_request();
	test_different_http_methods();
	test_http_versions();
	test_uri_with_query_string();
	test_byte_by_byte_arrival();
	test_case_sensitivity();
	test_chunked_single_chunk();
	test_chunked_multiple_chunks();
	test_chunked_fragmented_arrival();
	test_chunked_with_extensions();
	test_chunked_with_trailers();
	test_chunked_invalid_hex();
	test_chunked_overrides_content_length();
}

bool test_simple_get_request()
{
	HTTPRequest req;
	bool		all_tests_passed = true;
	const char *request			 = "GET /index.html HTTP/1.1\r\n"
								   "Host: localhost:8080\r\n"
								   "User-Agent: TestClient/1.0\r\n"
								   "\r\n";

	if (!req.parse(request))
	{
		std::cout << "test failed: request should be complete\n";
		all_tests_passed = false;
	}

	if (req.get_method() != "GET")
	{
		std::cout << "test failed: method should be GET\n";
		all_tests_passed = false;
	}
	if (req.get_request_target() != "/index.html")
	{
		std::cout << "test failed: request target should be /index.html\n";
		all_tests_passed = false;
	}
	if (req.get_protocol() != "HTTP/1.1")
	{
		std::cout << "test failed: protocol should be HTTP/1.1\n";
		all_tests_passed = false;
	}
	if (req.get_header_value("Host") != "localhost:8080")
	{
		std::cout << "test failed: Host header should be localhost:8080\n";
		all_tests_passed = false;
	}
	if (req.get_header_value("User-Agent") != "TestClient/1.0")
	{
		std::cout << "test failed: User-Agent header should be "
					 "TestClient/1.0\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_simple_get_request passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_simple_get_request failed\n";
	}
	return all_tests_passed;
}

bool test_packet_fragmentation()
{
	HTTPRequest req;
	bool		all_tests_passed = true;

	const char *chunk1 = "GET /test";
	const char *chunk2 = ".html HTTP/1.1\r\n";
	const char *chunk3 = "Host: exam";
	const char *chunk4 = "ple.com\r\n";
	const char *chunk5 = "Accept: */*\r\n";
	const char *chunk6 = "\r\n";

	if (req.parse(chunk1))
	{
		std::cout << "test failed: chunk1 should be incomplete\n";
		all_tests_passed = false;
	}
	if (req.parse(chunk2))
	{
		std::cout << "test failed: chunk2 should be incomplete\n";
		all_tests_passed = false;
	}
	if (req.parse(chunk3))
	{
		std::cout << "test failed: chunk3 should be incomplete\n";
		all_tests_passed = false;
	}
	if (req.parse(chunk4))
	{
		std::cout << "test failed: chunk4 should be incomplete\n";
		all_tests_passed = false;
	}
	if (req.parse(chunk5))
	{
		std::cout << "test failed: chunk5 should be incomplete\n";
		all_tests_passed = false;
	}
	if (!req.parse(chunk6))
	{
		std::cout << "test failed: chunk6 should be complete\n";
		all_tests_passed = false;
	}

	if (req.get_method() != "GET")
	{
		std::cout << "test failed: method should be GET\n";
		all_tests_passed = false;
	}
	if (req.get_request_target() != "/test.html")
	{
		std::cout << "test failed: request target should be /test.html\n";
		all_tests_passed = false;
	}
	if (req.get_header_value("Host") != "example.com")
	{
		std::cout << "test failed: Host header should be example.com\n";
		all_tests_passed = false;
	}
	if (req.get_header_value("Accept") != "*/*")
	{
		std::cout << "test failed: Accept header should be */*\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_packet_fragmentation passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_packet_fragmentation failed\n";
	}
	return all_tests_passed;
}

bool test_post_with_body()
{
	HTTPRequest req;
	bool		all_tests_passed = true;
	const char *request			 = "POST /api/users HTTP/1.1\r\n"
								   "Host: api.example.com\r\n"
								   "Content-Type: application/json\r\n"
								   "Content-Length: 24\r\n"
								   "\r\n"
								   "{\"name\":\"John\",\"age\":30}";

	if (!req.parse(request))
	{
		std::cout << "test failed: request should be complete\n";
		all_tests_passed = false;
	}

	if (req.get_method() != "POST")
	{
		std::cout << "test failed: method should be POST\n";
		all_tests_passed = false;
	}
	if (req.get_request_target() != "/api/users")
	{
		std::cout << "test failed: request target should be /api/users\n";
		all_tests_passed = false;
	}
	if (req.get_header_value("Content-Type") != "application/json")
	{
		std::cout << "test failed: Content-Type header should be "
					 "application/json\n";
		all_tests_passed = false;
	}
	if (req.get_header_value("Content-Length") != "24")
	{
		std::cout << "test failed: Content-Length header should be 24\n";
		all_tests_passed = false;
	}
	if (req.get_body() != "{\"name\":\"John\",\"age\":30}")
	{
		std::cout << "test failed: body should be "
					 "{\"name\":\"John\",\"age\":30}\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_post_with_body passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_post_with_body failed\n";
	}
	return all_tests_passed;
}

bool test_post_body_in_chunks()
{
	HTTPRequest req;
	bool		all_tests_passed = true;

	const char *headers = "POST /upload HTTP/1.1\r\n"
						  "Host: localhost\r\n"
						  "Content-Length: 17\r\n"
						  "\r\n";

	if (req.parse(headers))
	{
		std::cout << "test failed: headers parse should be incomplete (waiting "
					 "for "
					 "body)\n";
		all_tests_passed = false;
	}

	const char *bodyPart1 = "Hello, ";
	const char *bodyPart2 = "World!";
	const char *bodyPart3 = " 123";

	if (req.parse(bodyPart1))
	{
		std::cout << "test failed: bodyPart1 should be incomplete\n";
		all_tests_passed = false;
	}
	if (req.parse(bodyPart2))
	{
		std::cout << "test failed: bodyPart2 should be incomplete\n";
		all_tests_passed = false;
	}
	if (!req.parse(bodyPart3))
	{
		std::cout << "test failed: bodyPart3 should complete the request\n";
		all_tests_passed = false;
	}

	if (req.get_body() != "Hello, World! 123")
	{
		std::cout << "test failed: body should be 'Hello, World! 123'\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_post_body_in_chunks passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_post_body_in_chunks failed\n";
	}
	return all_tests_passed;
}

bool test_invalid_request_line()
{
	bool		all_tests_passed = true;

	// Case 1: Missing HTTP version
	HTTPRequest req1;
	const char *badRequest1 = "GET /index.html\r\n\r\n";
	if (req1.parse(badRequest1))
	{
		std::cout << "test failed: badRequest1 should return false\n";
		all_tests_passed = false;
	}
	// The parser should be in error state because it couldn't find the second
	// space
	if (!req1.is_error())
	{
		std::cout << "test failed: badRequest1 should set error state\n";
		all_tests_passed = false;
	}

	// Case 2: Garbage input
	HTTPRequest req2;
	const char *badRequest2 = "NOT_A_VALID_REQUEST_LINE\r\n\r\n";
	if (req2.parse(badRequest2))
	{
		std::cout << "test failed: badRequest2 should return false\n";
		all_tests_passed = false;
	}
	if (!req2.is_error())
	{
		std::cout << "test failed: badRequest2 should set error state\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_invalid_request_line passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_invalid_request_line failed\n";
	}
	return all_tests_passed;
}

bool test_header_without_colon()
{
	HTTPRequest req;
	bool		all_tests_passed = true;
	const char *request			 = "GET /test HTTP/1.1\r\n"
								   "Host localhost\r\n" // Missing colon!
						  "\r\n";

	if (req.parse(request))
	{
		std::cout << "test failed: request with malformed header should fail\n";
		all_tests_passed = false;
	}

	if (!req.is_error())
	{
		std::cout << "test failed: malformed header should trigger error "
					 "state\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_header_without_colon passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_header_without_colon failed\n";
	}
	return all_tests_passed;
}

bool test_multiple_headers()
{
	HTTPRequest req;
	bool		all_tests_passed = true;
	const char *request			 = "GET /test HTTP/1.1\r\n"
								   "Host: example.com\r\n"
								   "Accept: text/html\r\n"
								   "Accept: application/json\r\n" // Duplicate header
						  "Accept-Language: en-US\r\n"
						  "\r\n";

	if (!req.parse(request))
	{
		std::cout << "test failed: valid request failed to parse\n";
		all_tests_passed = false;
	}

	// Current behavior: last header wins (overwrites)
	// Ideally (RFC): should be "text/html, application/json"
	std::string accept = req.get_header_value("Accept");
	if (accept != "application/json")
	{
		std::cout << "test failed: expected 'application/json', got '" << accept
				  << "'\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_multiple_headers passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_multiple_headers failed\n";
	}
	return all_tests_passed;
}

bool test_header_value_with_spaces()
{
	HTTPRequest req;
	bool		all_tests_passed = true;
	const char *request			 = "GET /test HTTP/1.1\r\n"
								   "Host:   localhost:8080  \r\n"
								   "User-Agent:     Mozilla/5.0\r\n"
								   "\r\n";

	req.parse(request);

	std::string host = req.get_header_value("Host");
	if (host.find("localhost:8080") == std::string::npos)
	{
		std::cout << "test failed: Host header value '" << host
				  << "' should contain localhost:8080\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_header_value_with_spaces passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_header_value_with_spaces failed\n";
	}
	return all_tests_passed;
}

bool test_empty_uri()
{
	HTTPRequest req;
	bool		all_tests_passed = true;
	const char *request			 = "GET  HTTP/1.1\r\n" // Empty URI
						  "Host: localhost\r\n"
						  "\r\n";

	if (req.parse(request))
	{
		std::cout << "test failed: empty URI should fail\n";
		all_tests_passed = false;
	}

	if (!req.is_error())
	{
		std::cout << "test failed: empty URI should trigger error state\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET << " test_empty_uri passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET << " test_empty_uri failed\n";
	}
	return all_tests_passed;
}

bool test_large_request()
{
	HTTPRequest req;
	bool		all_tests_passed = true;
	std::string request			 = "POST /api/data HTTP/1.1\r\n"
								   "Host: api.example.com\r\n"
								   "User-Agent: TestClient/1.0\r\n"
								   "Accept: */*\r\n"
								   "Accept-Encoding: gzip, deflate\r\n"
								   "Accept-Language: en-US,en;q=0.9\r\n"
								   "Cache-Control: no-cache\r\n"
								   "Connection: keep-alive\r\n"
								   "Content-Type: application/x-www-form-urlencoded\r\n"
								   "Content-Length: 100\r\n"
								   "Cookie: session=abc123; user=johndoe\r\n"
								   "Origin: http://example.com\r\n"
								   "Referer: http://example.com/page\r\n"
								   "\r\n";

	// Add 100 bytes of body
	for (int i = 0; i < 100; i++)
	{
		request += 'A';
	}

	if (!req.parse(request))
	{
		std::cout << "test failed: large request should be complete\n";
		all_tests_passed = false;
	}
	if (req.get_body().length() != 100)
	{
		std::cout << "test failed: body length should be 100\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_large_request passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_large_request failed\n";
	}
	return all_tests_passed;
}

bool test_different_http_methods()
{
	bool		all_tests_passed = true;
	const char *methods[]		 = {"GET", "POST", "DELETE", "HEAD"};

	for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); i++)
	{
		HTTPRequest req;
		std::string request
			= std::string(methods[i]) + " /test HTTP/1.1\r\n\r\n";
		req.parse(request);
		if (req.get_method() != methods[i])
		{
			std::cout << "test failed: method should be " << methods[i] << "\n";
			all_tests_passed = false;
		}
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_different_http_methods passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_different_http_methods failed\n";
	}
	return all_tests_passed;
}

bool test_http_versions()
{
	bool		all_tests_passed = true;
	HTTPRequest req1;
	const char *http10 = "GET /test HTTP/1.0\r\n\r\n";
	req1.parse(http10);
	if (req1.get_protocol() != "HTTP/1.0")
	{
		std::cout << "test failed: protocol should be HTTP/1.0\n";
		all_tests_passed = false;
	}

	HTTPRequest req2;
	const char *http11 = "GET /test HTTP/1.1\r\n\r\n";
	req2.parse(http11);
	if (req2.get_protocol() != "HTTP/1.1")
	{
		std::cout << "test failed: protocol should be HTTP/1.1\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_http_versions passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_http_versions failed\n";
	}
	return all_tests_passed;
}

bool test_uri_with_query_string()
{
	HTTPRequest req;
	bool		all_tests_passed = true;
	const char *request = "GET /search?q=test&page=1&sort=asc HTTP/1.1\r\n"
						  "Host: example.com\r\n"
						  "\r\n";

	req.parse(request);
	if (req.get_request_target() != "/search?q=test&page=1&sort=asc")
	{
		std::cout << "test failed: request target mismatch\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_uri_with_query_string passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_uri_with_query_string failed\n";
	}
	return all_tests_passed;
}

bool test_byte_by_byte_arrival()
{
	HTTPRequest req;
	bool		all_tests_passed = true;
	const char *request			 = "GET / HTTP/1.1\r\nHost: test\r\n\r\n";

	size_t		len		 = strlen(request);
	bool		complete = false;

	// Feed one byte at a time
	for (size_t i = 0; i < len; i++)
	{
		complete = req.parse(std::string(1, request[i]));
		if (i < len - 1)
		{
			if (complete)
			{
				std::cout << "test failed: should not be complete at byte " << i
						  << "\n";
				all_tests_passed = false;
			}
		}
	}

	if (!complete)
	{
		std::cout << "test failed: should be complete after all bytes\n";
		all_tests_passed = false;
	}
	if (req.get_method() != "GET")
	{
		std::cout << "test failed: method should be GET\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_byte_by_byte_arrival passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_byte_by_byte_arrival failed\n";
	}
	return all_tests_passed;
}

bool test_case_sensitivity()
{
	HTTPRequest req;
	bool		all_tests_passed = true;
	const char *request			 = "GET /test HTTP/1.1\r\n"
								   "host: example.com\r\n"
								   "Content-Type: text/html\r\n"
								   "ACCEPT: */*\r\n"
								   "\r\n";

	req.parse(request);

	if (req.get_header_value("host") != "example.com"
		|| req.get_header_value("Host") != "example.com"
		|| req.get_header_value("HOST") != "example.com")
	{
		std::cout << "test failed: 'Host' header retrieval should be "
					 "case-insensitive\n";
		all_tests_passed = false;
	}

	if (req.get_header_value("content-type") != "text/html"
		|| req.get_header_value("Content-Type") != "text/html"
		|| req.get_header_value("CONTENT-TYPE") != "text/html")
	{
		std::cout << "test failed: 'Content-Type' header retrieval should be "
					 "case-insensitive\n";
		all_tests_passed = false;
	}

	if (req.get_header_value("accept") != "*/*"
		|| req.get_header_value("Accept") != "*/*"
		|| req.get_header_value("ACCEPT") != "*/*")
	{
		std::cout << "test failed: 'Accept' header retrieval should be "
					 "case-insensitive\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_case_sensitivity passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_case_sensitivity failed\n";
	}
	return all_tests_passed;
}

////////////////////////////////////////////////////// Chunked encoding tests //
bool test_chunked_single_chunk()
{
	HTTPRequest req;
	bool		all_tests_passed = true;

	// Single chunk of 5 bytes: "Hello"
	const char *request = "POST /upload HTTP/1.1\r\n"
						  "Host: localhost\r\n"
						  "Transfer-Encoding: chunked\r\n"
						  "\r\n"
						  "5\r\n"
						  "Hello\r\n"
						  "0\r\n"
						  "\r\n";

	if (!req.parse(request))
	{
		std::cout << "test failed: chunked request should be complete\n";
		all_tests_passed = false;
	}
	if (req.get_body() != "Hello")
	{
		std::cout << "test failed: body should be 'Hello', got '"
				  << req.get_body() << "'\n";
		all_tests_passed = false;
	}
	if (req.is_error())
	{
		std::cout << "test failed: should not be in error state\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_chunked_single_chunk passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_chunked_single_chunk failed\n";
	}
	return all_tests_passed;
}

bool test_chunked_multiple_chunks()
{
	HTTPRequest req;
	bool		all_tests_passed = true;

	// Three chunks: "Hello" (5) + " " (1) + "World!" (6) = "Hello World!"
	const char *request = "POST /data HTTP/1.1\r\n"
						  "Host: localhost\r\n"
						  "Transfer-Encoding: chunked\r\n"
						  "\r\n"
						  "5\r\n"
						  "Hello\r\n"
						  "1\r\n"
						  " \r\n"
						  "6\r\n"
						  "World!\r\n"
						  "0\r\n"
						  "\r\n";

	if (!req.parse(request))
	{
		std::cout << "test failed: chunked request should be complete\n";
		all_tests_passed = false;
	}
	if (req.get_body() != "Hello World!")
	{
		std::cout << "test failed: body should be 'Hello World!', got '"
				  << req.get_body() << "'\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_chunked_multiple_chunks passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_chunked_multiple_chunks failed\n";
	}
	return all_tests_passed;
}

bool test_chunked_fragmented_arrival()
{
	HTTPRequest req;
	bool		all_tests_passed = true;

	// Simulate data arriving in fragments across multiple recv() calls
	const char *part1 = "POST /upload HTTP/1.1\r\n"
						"Host: localhost\r\n"
						"Transfer-Encoding: chunked\r\n"
						"\r\n"
						"5\r\n"
						"Hel";

	const char *part2 = "lo\r\n"
						"8\r\n"
						" Wo";

	const char *part3 = "rld!\n\r\n"
						"0\r\n"
						"\r\n";

	if (req.parse(part1))
	{
		std::cout << "test failed: part1 should be incomplete\n";
		all_tests_passed = false;
	}
	if (req.parse(part2))
	{
		std::cout << "test failed: part2 should be incomplete\n";
		all_tests_passed = false;
	}
	if (!req.parse(part3))
	{
		std::cout << "test failed: part3 should complete the request\n";
		all_tests_passed = false;
	}
	if (req.get_body() != "Hello World!\n")
	{
		std::cout << "test failed: body should be 'Hello World!\\n', got '"
				  << req.get_body() << "'\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_chunked_fragmented_arrival passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_chunked_fragmented_arrival failed\n";
	}
	return all_tests_passed;
}

bool test_chunked_with_extensions()
{
	HTTPRequest req;
	bool		all_tests_passed = true;

	// Chunk size line with extensions (;name=value) that must be ignored
	const char *request = "POST /data HTTP/1.1\r\n"
						  "Host: localhost\r\n"
						  "Transfer-Encoding: chunked\r\n"
						  "\r\n"
						  "a;ext=val\r\n"
						  "0123456789\r\n"
						  "0\r\n"
						  "\r\n";

	if (!req.parse(request))
	{
		std::cout << "test failed: chunked with extensions should complete\n";
		all_tests_passed = false;
	}
	if (req.get_body() != "0123456789")
	{
		std::cout << "test failed: body should be '0123456789', got '"
				  << req.get_body() << "'\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_chunked_with_extensions passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_chunked_with_extensions failed\n";
	}
	return all_tests_passed;
}

bool test_chunked_with_trailers()
{
	HTTPRequest req;
	bool		all_tests_passed = true;

	// Trailers after the final 0-chunk should be consumed and ignored
	const char *request = "POST /data HTTP/1.1\r\n"
						  "Host: localhost\r\n"
						  "Transfer-Encoding: chunked\r\n"
						  "\r\n"
						  "4\r\n"
						  "Wiki\r\n"
						  "5\r\n"
						  "pedia\r\n"
						  "0\r\n"
						  "Expires: Wed, 21 Oct 2015 07:28:00 GMT\r\n"
						  "\r\n";

	if (!req.parse(request))
	{
		std::cout << "test failed: chunked with trailers should complete\n";
		all_tests_passed = false;
	}
	if (req.get_body() != "Wikipedia")
	{
		std::cout << "test failed: body should be 'Wikipedia', got '"
				  << req.get_body() << "'\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_chunked_with_trailers passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_chunked_with_trailers failed\n";
	}
	return all_tests_passed;
}

bool test_chunked_invalid_hex()
{
	HTTPRequest req;
	bool		all_tests_passed = true;

	// "XZ" is not valid hex -> should trigger error
	const char *request = "POST /data HTTP/1.1\r\n"
						  "Host: localhost\r\n"
						  "Transfer-Encoding: chunked\r\n"
						  "\r\n"
						  "XZ\r\n"
						  "ab\r\n"
						  "0\r\n"
						  "\r\n";

	if (req.parse(request))
	{
		std::cout << "test failed: invalid hex should not complete\n";
		all_tests_passed = false;
	}
	if (!req.is_error())
	{
		std::cout << "test failed: invalid hex should trigger error state\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_chunked_invalid_hex passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_chunked_invalid_hex failed\n";
	}
	return all_tests_passed;
}

bool test_chunked_overrides_content_length()
{
	HTTPRequest req;
	bool		all_tests_passed = true;

	// Both Content-Length and Transfer-Encoding present:
	// chunked must take precedence (RFC 7230 Section 3.3.3)
	const char *request = "POST /data HTTP/1.1\r\n"
						  "Host: localhost\r\n"
						  "Content-Length: 999\r\n"
						  "Transfer-Encoding: chunked\r\n"
						  "\r\n"
						  "3\r\n"
						  "abc\r\n"
						  "0\r\n"
						  "\r\n";

	if (!req.parse(request))
	{
		std::cout << "test failed: chunked should override content-length\n";
		all_tests_passed = false;
	}
	if (req.get_body() != "abc")
	{
		std::cout << "test failed: body should be 'abc', got '"
				  << req.get_body() << "'\n";
		all_tests_passed = false;
	}

	if (all_tests_passed)
	{
		std::cout << BG_GREEN << " OK " << RESET
				  << " test_chunked_overrides_content_length passed\n";
	}
	else
	{
		std::cout << BG_RED << " KO " << RESET
				  << " test_chunked_overrides_content_length failed\n";
	}
	return all_tests_passed;
}
