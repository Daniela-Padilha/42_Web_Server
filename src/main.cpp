#include "../inc/webserver.hpp"

int main(int argc, char **argv)
{
#ifdef TESTING
	(void) argc;
	(void) argv;
	test_http_parser();
	test_http_response();
	test_http_features();
	return (EXIT_SUCCESS);
#endif

	init_signals();

	std::string config_path;
	Config		config;

	if (!init_args(argc, argv, config_path)
		|| !init_config(config_path, config))
	{
		return (EXIT_FAILURE);
	}

	std::vector<ServerConfig> parsed_config = config.get_server();
	HTTPHandler::set_error_pages(parsed_config.error_pages);

	Server server(parsed_config);
	server.start();

	dprint("end of main");
	return (EXIT_SUCCESS);
}
