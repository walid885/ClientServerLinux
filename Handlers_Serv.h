#ifndef HANDLERS_SERV_H
#define HANDLERS_SERV_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <signal.h>
#include <stdio.h>

// Variable globale pour indiquer que le serveur doit continuer
volatile sig_atomic_t encore = 0;

/**
 * Gestionnaire du signal SIGUSR1 pour le réveil du serveur.
 * @param sig Le signal reçu (SIGUSR1).
 */
void hand_reveil(int sig) {
    if (sig == SIGUSR1) {
        encore = 1;
    }
}

/**
 * Gestionnaire des signaux pour la fin du serveur.
 * @param sig Le signal reçu (peu importe lequel).
 */
void fin_serveur(int sig) {
    (void)sig;  // Pour éviter l'avertissement de paramètre non utilisé
    encore = 0;
}

#endif // HANDLERS_SERV_H