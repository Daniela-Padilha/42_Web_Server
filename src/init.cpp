#include "../inc/init.hpp"

bool init_args(int argc, char **argv, std::string &config_path)
{
	if (argc <= 1)
	{
		config_path = "default.conf";
	}
	else if (argc == 2)
	{
		config_path = argv[1];
	}
	else
	{
		std::cerr << "Usage: ./webserv [config_file]" << '\n';
		return (false);
	}
	return (true);
}

bool init_config(const std::string &config_path, Config &config)
{
	if (!config.parse(config_path))
	{
		std::cerr << "Config error: " << config.error() << '\n';
		return (false);
	}
	return (true);
}
