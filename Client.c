#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "serv_cli_fifo.h"

// Variable globale pour le suivi
volatile sig_atomic_t signal_recu = 0;

// Gestionnaire de signal pour SIGUSR1
void handle_sigusr1(int signo) {
    if (signo == SIGUSR1) {
        printf("[CLIENT %d][%ld] Signal SIGUSR1 reçu\n", getpid(), time(NULL));
        signal_recu = 1;
    }
}

// Fonction pour afficher l'heure actuelle
void print_timestamp() {
    time_t now = time(NULL);
    printf("[%ld] ", now);
}

int main(int argc, char *argv[]) {
    int fifo1, fifo2;
    question_t question;
    reponse_t reponse;

    // Configuration du gestionnaire de signal
    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        print_timestamp();
        perror("[CLIENT] Erreur configuration signal SIGUSR1");
        exit(EXIT_FAILURE);
    }

    print_timestamp();
    printf("[CLIENT %d] Démarrage du client\n", getpid());

    // Initialisation du générateur de nombres aléatoires
    srand(time(NULL) + getpid());

    // Ouverture du tube FIFO1 (écriture)
    print_timestamp();
    printf("[CLIENT %d] Tentative d'ouverture du tube FIFO1 (%s)\n", getpid(), FIFO1);
    
    fifo1 = open(FIFO1, O_WRONLY);
    if (fifo1 == -1) {
        print_timestamp();
        perror("[CLIENT] Échec ouverture FIFO1");
        exit(EXIT_FAILURE);
    }
    print_timestamp();
    printf("[CLIENT %d] FIFO1 ouvert avec succès (fd=%d)\n", getpid(), fifo1);

    // Ouverture du tube FIFO2 (lecture)
    print_timestamp();
    printf("[CLIENT %d] Tentative d'ouverture du tube FIFO2 (%s)\n", getpid(), FIFO2);
    
    fifo2 = open(FIFO2, O_RDONLY);
    if (fifo2 == -1) {
        print_timestamp();
        perror("[CLIENT] Échec ouverture FIFO2");
        close(fifo1);
        exit(EXIT_FAILURE);
    }
    print_timestamp();
    printf("[CLIENT %d] FIFO2 ouvert avec succès (fd=%d)\n", getpid(), fifo2);

    // Préparation de la question
    question.client_id = getpid();
    question.n = rand() % NMAX + 1;

    print_timestamp();
    printf("[CLIENT %d] Préparation de la demande: %d nombres aléatoires\n", 
           question.client_id, question.n);

    // Envoi de la question
    ssize_t write_result = write(fifo1, &question, sizeof(question_t));
    if (write_result == -1) {
        print_timestamp();
        perror("[CLIENT] Erreur envoi question");
        close(fifo1);
        close(fifo2);
        exit(EXIT_FAILURE);
    }
    print_timestamp();
    printf("[CLIENT %d] Question envoyée avec succès (%zd bytes)\n", 
           question.client_id, write_result);

    // Attente du signal
    print_timestamp();
    printf("[CLIENT %d] En attente du signal du serveur...\n", question.client_id);
    
    while (!signal_recu) {
        pause();
    }

    // Lecture de la réponse
    print_timestamp();
    printf("[CLIENT %d] Tentative de lecture de la réponse\n", question.client_id);
    
    ssize_t read_result = read(fifo2, &reponse, sizeof(reponse_t));
    if (read_result == -1) {
        print_timestamp();
        perror("[CLIENT] Erreur lecture réponse");
        close(fifo1);
        close(fifo2);
        exit(EXIT_FAILURE);
    }
    print_timestamp();
    printf("[CLIENT %d] Réponse reçue (%zd bytes)\n", question.client_id, read_result);

    // Affichage de la réponse
    print_timestamp();
    printf("[CLIENT %d] Nombres reçus (%d): ", reponse.client_id, reponse.n);
    for (int i = 0; i < reponse.n; i++) {
        printf("%d ", reponse.nombres[i]);
    }
    printf("\n");

    // Envoi du signal de confirmation au serveur
    print_timestamp();
    printf("[CLIENT %d] Envoi du signal de confirmation au serveur\n", question.client_id);
    
    if (kill(reponse.client_id, SIGUSR1) == -1) {
        print_timestamp();
        perror("[CLIENT] Erreur envoi signal confirmation");
    } else {
        print_timestamp();
        printf("[CLIENT %d] Signal de confirmation envoyé avec succès\n", question.client_id);
    }

    // Fermeture des tubes
    print_timestamp();
    printf("[CLIENT %d] Fermeture des tubes\n", question.client_id);
    
    close(fifo1);
    close(fifo2);

    print_timestamp();
    printf("[CLIENT %d] Terminaison du client\n", question.client_id);

    return EXIT_SUCCESS;
}