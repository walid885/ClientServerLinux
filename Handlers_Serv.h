// handlers_serv.h
#ifndef HANDLERS_SERV_H
#define HANDLERS_SERV_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <signal.h>
#include <stdio.h>

// Global variable to indicate server should continue
volatile sig_atomic_t encore = 0;
volatile sig_atomic_t running = 1;

/**
 * Handler for SIGUSR1 signal to wake up the server.
 * @param sig Received signal (SIGUSR1).
 */
void hand_reveil(int sig) {
    if (sig == SIGUSR1) {
        encore = 1;
    }
}

/**
 * Signal handler for server termination.
 * @param sig Received signal.
 */
void fin_serveur(int sig) {
    (void)sig;  // Avoid unused parameter warning
    running = 0;
    encore = 0;
}

#endif // HANDLERS_SERV_H