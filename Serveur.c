#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include "serv_cli_fifo.h"
#include "Handlers_Serv.h"

// Structure pour suivre les clients actifs
typedef struct {
    pid_t pid;
    int actif;
} client_info_t;

#define MAX_CLIENTS 10
client_info_t clients[MAX_CLIENTS];
volatile sig_atomic_t nb_clients = 0;

// Gestionnaire pour SIGCHLD pour gérer la terminaison des processus fils
void handle_sigchld(int sig) {
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].pid == pid) {
                clients[i].actif = 0;
                nb_clients--;
                printf("Serveur : Client %d terminé\n", pid);
                break;
            }
        }
    }
}

// Fonction pour traiter un client
void traiter_client(int fifo1, int fifo2) {
    question_t question;
    reponse_t reponse;
    
    while (1) {
        // Lire la question
        ssize_t read_bytes = read(fifo1, &question, sizeof(question_t));
        if (read_bytes == -1) {
            if (errno == EINTR) continue;
            perror("Erreur lecture question");
            return;
        }
        
        printf("Serveur [%d] : Question reçue du client %d (demande %d nombres)\n", 
               getpid(), question.client_id, question.n);
        
        // Préparer la réponse
        reponse.client_id = getpid();
        reponse.n = question.n;
        for (int i = 0; i < question.n; i++) {
            reponse.nombres[i] = rand() % 100;
        }
        
        // Écrire la réponse
        printf("Serveur [%d] : Écriture de la réponse pour client %d\n", 
               getpid(), question.client_id);
        if (write(fifo2, &reponse, sizeof(reponse_t)) == -1) {
            perror("Erreur écriture réponse");
            continue;
        }
        
        // Signaler au client
        printf("Serveur [%d] : Réveil du client %d\n", getpid(), question.client_id);
        if (kill(question.client_id, SIGUSR1) == -1) {
            perror("Erreur envoi signal");
            continue;
        }
        
        // Attendre la confirmation du client
        encore = 0;
        while (!encore) {
            pause();
        }
        
        printf("Serveur [%d] : Confirmation reçue du client %d\n", 
               getpid(), question.client_id);
    }
}

int main() {
    int fifo1, fifo2;
    
    // Initialisation du tableau des clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].actif = 0;
    }
    
    // Configuration du gestionnaire SIGCHLD
    struct sigaction sa_chld;
    sa_chld.sa_handler = handle_sigchld;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("Erreur installation handler SIGCHLD");
        exit(1);
    }
    
    // Supprime les anciens FIFOs
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

    printf("Serveur principal [%d] : Démarrage...\n", getpid());
    
    // Initialiser le générateur de nombres aléatoires
    srand(time(NULL));
    
    // Ouvrir les tubes nommés
    printf("Serveur : Ouverture des FIFOs...\n");
    
    fifo1 = open(FIFO1, O_RDONLY | O_NONBLOCK);
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
    sa.sa_flags = SA_RESTART;
    
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Erreur installation handler SIGUSR1");
        exit(1);
    }
    
    sa.sa_handler = fin_serveur;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Erreur installation handler SIGINT");
        exit(1);
    }
    
    // Boucle principale pour accepter de nouveaux clients
    while (1) {
        if (nb_clients < MAX_CLIENTS) {
            pid_t pid = fork();
            
            if (pid == -1) {
                perror("Erreur fork");
                continue;
            }
            
            if (pid == 0) {  // Processus fils
                // Configuration spécifique au processus fils
                printf("Serveur : Nouveau processus fils créé [%d]\n", getpid());
                traiter_client(fifo1, fifo2);
                exit(0);
            } else {  // Processus parent
                // Enregistrer le nouveau client
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (!clients[i].actif) {
                        clients[i].pid = pid;
                        clients[i].actif = 1;
                        nb_clients++;
                        break;
                    }
                }
            }
        }
        
        // Attente courte avant de vérifier à nouveau
        sleep(1);
    }
    
    // Nettoyage
    close(fifo1);
    close(fifo2);
    unlink(FIFO1);
    unlink(FIFO2);
    
    return 0;
}