#ifndef HANDLERS_SERV_H
#define HANDLERS_SERV_H

#include <signal.h>

// Variable globale pour indiquer que le serveur doit continuer
static int encore=0 ;
/**
 * Gestionnaire du signal SIGUSR1 pour le réveil du serveur.
 * @param sig Le signal reçu (SIGUSR1).
 */
void hand_reveil(int sig) {
    // Indique que le serveur doit continuer
    encore = 1;
}

/**
 * Gestionnaire des signaux pour la fin du serveur.
 * @param sig Le signal reçu (peu importe lequel).
 */
void fin_serveur(int sig) {
    // Indique que le serveur doit s'arrêter
    encore = 0;
}

#endif // HANDLERS_SERV_H