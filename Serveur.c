#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "serv_cli_fifo.h"
#include "Handlers_Serv.h"

int main() {
    int fifo1, fifo2;
    question_t question;
    reponse_t reponse;
    
    // Supprime les anciens FIFOs s'ils existent
    unlink(FIFO1);
    unlink(FIFO2);
    
    // Créer les tubes nommés
    if (mkfifo(FIFO1, 0666) == -1) {
        perror("Erreur création FIFO1");
        exit(1);
    }
    if (mkfifo(FIFO2, 0666) == -1) {
        perror("Erreur création FIFO2");
        unlink(FIFO1);
        exit(1);
    }

    printf("Serveur [%d] : Démarrage...\n", getpid());
    
    // Initialiser le générateur de nombres aléatoires
    srand(getpid());
    
    // Ouvrir les tubes nommés
    printf("Serveur : Ouverture des FIFOs...\n");
    
    fifo1 = open(FIFO1, O_RDONLY);
    if (fifo1 == -1) {
        perror("Erreur ouverture FIFO1");
        exit(1);
    }
    
    fifo2 = open(FIFO2, O_WRONLY);
    if (fifo2 == -1) {
        perror("Erreur ouverture FIFO2");
        close(fifo1);
        exit(1);
    }
    
    // Installer les gestionnaires de signaux
    struct sigaction sa;
    sa.sa_handler = hand_reveil;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Erreur installation handler SIGUSR1");
        exit(1);
    }
    
    sa.sa_handler = fin_serveur;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Erreur installation handler SIGINT");
        exit(1);
    }
    
    // Boucle principale
    while (1) {
        printf("Serveur : En attente d'une question dans FIFO1...\n");
        
        // Lire la question
        ssize_t read_bytes = read(fifo1, &question, sizeof(question_t));
        if (read_bytes == -1) {
            perror("Erreur lecture question");
            continue;
        }
        
        printf("Serveur : Question reçue du client %d (demande %d nombres)\n", 
               question.client_id, question.n);
        
        // Préparer la réponse
        reponse.client_id = getpid();  // ID du serveur
        reponse.n = question.n;
        for (int i = 0; i < question.n; i++) {
            reponse.nombres[i] = rand() % 100;
        }
        
        // Écrire la réponse
        printf("Serveur : Écriture de la réponse dans FIFO2...\n");
        if (write(fifo2, &reponse, sizeof(reponse_t)) == -1) {
            perror("Erreur écriture réponse");
            continue;
        }
        
        // Signaler au client
        printf("Serveur : Réveil du client %d\n", question.client_id);
        if (kill(question.client_id, SIGUSR1) == -1) {
            perror("Erreur envoi signal");
            continue;
        }
        
        // Attendre la confirmation du client
        encore = 0;
        while (!encore) {
            pause();
        }
        
        printf("Serveur : Confirmation reçue du client %d\n", question.client_id);
    }
    
    // Nettoyage
    close(fifo1);
    close(fifo2);
    unlink(FIFO1);
    unlink(FIFO2);
    
    return 0;
}