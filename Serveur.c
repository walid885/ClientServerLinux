#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "serv_cli_fifo.h"
#include "Handlers_Serv.h"

int main() {
    int fifo1, fifo2;
    question_t question;
    reponse_t reponse;

    // Créer les tubes nommés
    mkfifo(FIFO1, 0666);
    mkfifo(FIFO2, 0666);
srand(getpid());
    // Ouvrir les tubes nommés
    fifo1 = open(FIFO1, O_RDONLY);
    fifo2 = open(FIFO2, O_WRONLY);

    // Installer les gestionnaires de signaux
    signal(SIGUSR1, hand_reveil);
    signal(SIGINT, fin_serveur);

    while (1) {
        // Lire la question du client dans fifo1
        printf("Serveur : En attente d'une question dans FIFO1...\n");
        read(fifo1, &question, sizeof(question_t));
        printf("Serveur : Question reçue du client %d (demande %d nombres)\n", question.client_id, question.n);

        // Générer les nombres aléatoires
        reponse.client_id = question.client_id;
        reponse.n = question.n;
        for (int i = 0; i < reponse.n; i++) {
            reponse.nombres[i] = rand() % 100;
        }

        // Écrire la réponse dans fifo2
        printf("Serveur : Écriture de la réponse dans FIFO2...\n");
        write(fifo2, &reponse, sizeof(reponse_t));

        // Réveiller le client
        printf("Serveur : Réveil du client %d avec le signal SIGUSR1\n", question.client_id);
        kill(question.client_id, SIGUSR1);

        // Attendre le réveil du client
        printf("Serveur : En attente du réveil du client %d\n", question.client_id);
        pause();
    }

    return 0;
}