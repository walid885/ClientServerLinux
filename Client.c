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
#include <string.h>
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
    printf("[%ld timestamp] ", now);
}

int main(int argc, char *argv[]) {
    (void)argc;  // Suppress unused parameter warning
    (void)argv;  // Suppress unused parameter warning
    
    int registry_fd, fd_in, fd_out;
    question_t question;
    reponse_t reponse;
    char fifo_in[128], fifo_out[128];
    pid_t pid = getpid();

    // Construction des noms de FIFO pour ce client
    snprintf(fifo_in, sizeof(fifo_in), "%s%d_in", FIFO_BASE, pid);
    snprintf(fifo_out, sizeof(fifo_out), "%s%d_out", FIFO_BASE, pid);

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
    printf("[CLIENT %d] Démarrage du client\n", pid);

    // Création des FIFOs client
    if (mkfifo(fifo_in, 0666) == -1 && errno != EEXIST) {
        print_timestamp();
        perror("[CLIENT] Erreur création FIFO in");
        exit(EXIT_FAILURE);
    }

    if (mkfifo(fifo_out, 0666) == -1 && errno != EEXIST) {
        print_timestamp();
        perror("[CLIENT] Erreur création FIFO out");
        unlink(fifo_in);
        exit(EXIT_FAILURE);
    }

    // Ouverture du FIFO registry pour enregistrement auprès du serveur
    print_timestamp();
    printf("[CLIENT %d] Tentative d'ouverture du registry (%s)\n", pid, SERVER_REGISTRY);
    
    registry_fd = open(SERVER_REGISTRY, O_WRONLY);
    if (registry_fd == -1) {
        print_timestamp();
        perror("[CLIENT] Échec ouverture registry");
        unlink(fifo_in);
        unlink(fifo_out);
        exit(EXIT_FAILURE);
    }

    // Envoi du PID au serveur via le registry
    if (write(registry_fd, &pid, sizeof(pid_t)) == -1) {
        print_timestamp();
        perror("[CLIENT] Échec envoi PID au registry");
        close(registry_fd);
        unlink(fifo_in);
        unlink(fifo_out);
        exit(EXIT_FAILURE);
    }
    close(registry_fd);

    // Ouverture des FIFOs client
    print_timestamp();
    printf("[CLIENT %d] Tentative d'ouverture du FIFO in (%s)\n", pid, fifo_in);
    
    fd_in = open(fifo_in, O_RDONLY);
    if (fd_in == -1) {
        print_timestamp();
        perror("[CLIENT] Échec ouverture FIFO in");
        unlink(fifo_in);
        unlink(fifo_out);
        exit(EXIT_FAILURE);
    }

    print_timestamp();
    printf("[CLIENT %d] Tentative d'ouverture du FIFO out (%s)\n", pid, fifo_out);
    
    fd_out = open(fifo_out, O_WRONLY);
    if (fd_out == -1) {
        print_timestamp();
        perror("[CLIENT] Échec ouverture FIFO out");
        close(fd_in);
        unlink(fifo_in);
        unlink(fifo_out);
        exit(EXIT_FAILURE);
    }

    // Initialisation du générateur de nombres aléatoires
    srand(time(NULL) + pid);

    // Préparation de la question
    question.client_id = pid;
    question.n = rand() % NMAX + 1;

    print_timestamp();
    printf("[CLIENT %d] Préparation de la demande: %d nombres aléatoires\n", 
           question.client_id, question.n);

    // Envoi de la question
    ssize_t write_result = write(fd_out, &question, sizeof(question_t));
    if (write_result == -1) {
        print_timestamp();
        perror("[CLIENT] Erreur envoi question");
        close(fd_in);
        close(fd_out);
        unlink(fifo_in);
        unlink(fifo_out);
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
    
    ssize_t read_result = read(fd_in, &reponse, sizeof(reponse_t));
    if (read_result == -1) {
        print_timestamp();
        perror("[CLIENT] Erreur lecture réponse");
        close(fd_in);
        close(fd_out);
        unlink(fifo_in);
        unlink(fifo_out);
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

    // Nettoyage et fermeture
    print_timestamp();
    printf("[CLIENT %d] Fermeture des tubes\n", question.client_id);
    
    close(fd_in);
    close(fd_out);
    unlink(fifo_in);
    unlink(fifo_out);

    print_timestamp();
    printf("[CLIENT %d] Terminaison du client\n", question.client_id);

    return EXIT_SUCCESS;
}