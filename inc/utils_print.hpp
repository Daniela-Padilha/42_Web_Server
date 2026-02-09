#ifndef UTILS_PRINT_HPP
#define UTILS_PRINT_HPP

#include <iostream>

#ifdef DEBUG
# define dprint(msg) std::cout << "==DEBUG== " << msg << std::endl
# define eprint(msg) std::cout << "==ERROR== " << msg << std::endl
#else
# define dprint(msg) ((void) 0)
# define eprint(msg) ((void) 0)
# define DEBUG		 false
#endif

#define BG_GREEN	 "\033[42m"
#define BG_RED		 "\033[41m"
#define BG_BOLD_BLUE "\033[1;30m\033[106m"
#define RESET		 "\033[0m"

#endif
