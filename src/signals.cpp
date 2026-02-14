#include "../inc/signals.hpp"

volatile sig_atomic_t g_looping = 1;

static void			  handle_signals(int sig);

void				  init_signals()
{
	struct sigaction s_a;
	s_a.sa_handler = handle_signals;
	s_a.sa_flags   = 0;
	sigemptyset(&s_a.sa_mask);

	sigaction(SIGINT, &s_a, NULL);
	sigaction(SIGQUIT, &s_a, NULL);
	sigaction(SIGTERM, &s_a, NULL);
}

static void handle_signals(int sig)
{
	(void) sig;
	std::cout << "  Signal received. Exiting...\n";
	g_looping = 0;
}
