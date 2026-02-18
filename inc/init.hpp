#ifndef INIT_HPP
#define INIT_HPP

#include <iostream>
#include <string>

#include "../inc/Config.hpp"

bool init_args(int argc, char **argv, std::string &config_path);
bool init_config(const std::string &config_path, Config &config);

#endif
