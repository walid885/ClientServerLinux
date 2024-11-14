#ifndef SERV_CLI_FIFO_H
#define SERV_CLI_FIFO_H

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

// Constantes
#define NMAX 100  // Nombre maximum de nombres aléatoires
#define FIFO1 "/tmp/fifo1"  // Chemin du premier tube nommé
#define FIFO2 "/tmp/fifo2"  // Chemin du second tube nommé

// Structure pour la question envoyée par le client
typedef struct {
    pid_t client_id;  // PID du client
    int n;            // Nombre de nombres aléatoires demandés
} question_t;

// Structure pour la réponse envoyée par le serveur
typedef struct {
    pid_t client_id;           // PID du client destinataire
    int n;                     // Nombre de nombres générés
    int nombres[NMAX];         // Tableau des nombres générés
} reponse_t;

#endif // SERV_CLI_FIFO_H