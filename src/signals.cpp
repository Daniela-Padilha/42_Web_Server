#include "../inc/signals.hpp"
#include <unistd.h>

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

	// Ignore SIGPIPE — writing to a closed socket should return EPIPE,
	// not kill the server.  Critical under high concurrency.
	struct sigaction s_ign;
	s_ign.sa_handler = SIG_IGN;
	s_ign.sa_flags	 = 0;
	sigemptyset(&s_ign.sa_mask);
	sigaction(SIGPIPE, &s_ign, NULL);
}

static void handle_signals(int sig)
{
	(void) sig;
	write(STDOUT_FILENO, "\nServer shutting down...\n", 25);
	g_looping = 0;
}
