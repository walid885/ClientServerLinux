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

int main(int argc, char *argv[]) {
    int fifo1, fifo2;
    question_t question;
    reponse_t reponse;

    // Ouvrir les tubes nommés
    fifo1 = open(FIFO1, O_WRONLY);
    fifo2 = open(FIFO2, O_RDONLY);

    // Envoyer la question au serveur
    question.client_id = getpid();
    question.n = rand() % NMAX + 1;
    write(fifo1, &question, sizeof(question_t));

    // Attendre le réveil du serveur
    pause();

    // Lire la réponse du serveur
    read(fifo2, &reponse, sizeof(reponse_t));

    // Afficher la réponse
    printf("Client %d a reçu %d nombres aléatoires : ", reponse.client_id, reponse.n);
    for (int i = 0; i < reponse.n; i++) {
        printf("%d ", reponse.nombres[i]);
    }
    printf("\n");

    // Avertir le serveur
    kill(reponse.client_id, SIGUSR1);

    return 0;
}