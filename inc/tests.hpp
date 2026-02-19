#ifndef TESTS_HPP
#define TESTS_HPP

#include "../inc/HTTPRequest.hpp"
#include "../inc/HTTPResponse.hpp"

#include <cstring>
#include <iostream>

void test_http_parser();

bool test_simple_get_request();
bool test_packet_fragmentation();
bool test_post_with_body();
bool test_post_body_in_chunks();
bool test_invalid_request_line();
bool test_header_without_colon();
bool test_multiple_headers();
bool test_header_value_with_spaces();
bool test_empty_uri();
bool test_large_request();
bool test_different_http_methods();
bool test_http_versions();
bool test_uri_with_query_string();
bool test_byte_by_byte_arrival();
bool test_case_sensitivity();
bool test_chunked_single_chunk();
bool test_chunked_multiple_chunks();
bool test_chunked_fragmented_arrival();
bool test_chunked_with_extensions();
bool test_chunked_with_trailers();
bool test_chunked_invalid_hex();
bool test_chunked_overrides_content_length();

void test_http_response();

bool test_response_error_400();
bool test_response_error_403();
bool test_response_error_404();
bool test_response_error_405();
bool test_response_error_413();
bool test_response_error_500();
bool test_response_redirect_301();
bool test_response_redirect_302();
bool test_response_error_page_from_file();

#endif
