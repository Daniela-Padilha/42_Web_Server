#include "../inc/webserver.hpp"

int main(void)
{
	// eprint("Please make sure this line is commended.");
	init_signals();

	// test_http_parser();

	std::map<int, std::string> error_pages;
	error_pages[400] = "files/error_400.html";
	error_pages[404] = "files/error_404.html";
	error_pages[405] = "files/error_405.html";
	error_pages[500] = "files/error_500.html";
	HTTPHandler::set_error_pages(error_pages);

	Server server(8080);
	server.start();

	dprint("end of main");
	return (EXIT_SUCCESS);
}
