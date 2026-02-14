#include "../inc/webserver.hpp"

int main(void)
{
	// eprint("Please make sure this line is commended.");
	// test_http_parser();
	init_signals();

	Server server(8080);
	server.start();

	dprint("end of main");
	return (EXIT_SUCCESS);
}
