#ifndef SIGNALS_HPP
#define SIGNALS_HPP

#include <csignal>
#include <iostream>
#include <ostream>

extern volatile sig_atomic_t g_looping;
void						 init_signals();

#endif
