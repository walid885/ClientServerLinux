#ifndef SERV_CLI_FIFO_H
#define SERV_CLI_FIFO_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

// Constantes et macros communes
#define NMAX 10 // Nombre maximum de nombres aléatoires
#define FIFO1 "/tmp/fifo1" // Nom du tube nommé pour les questions
#define FIFO2 "/tmp/fifo2" // Nom du tube nommé pour les réponses

// Structure de la question
typedef struct {
    int client_id; // Numéro du client
    int n; // Nombre de nombres aléatoires demandés
} question_t;

// Structure de la réponse
typedef struct {
    int client_id; // Numéro du client
    int nombres[NMAX]; // Tableau des nombres aléatoires
    int n; // Nombre de nombres aléatoires
} reponse_t;

#endif // SERV_CLI_FIFO_H