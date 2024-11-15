// serv_cli_fifo.h
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

// Constants
#define NMAX 100              // Maximum number of random numbers
#define MAX_CLIENTS 10        // Maximum number of simultaneous clients
#define FIFO_BASE "/tmp/fifo_client_"  // Base path for client-specific FIFOs
#define SERVER_REGISTRY "/tmp/server_registry"  // Registry FIFO path

// Structure for client request
typedef struct {
    pid_t client_id;  // Client PID
    int n;            // Number of requested random numbers
} question_t;

// Structure for server response
typedef struct {
    pid_t client_id;    // Target client PID
    int n;              // Number of generated numbers
    int nombres[NMAX];  // Array of generated numbers
} reponse_t;

// Structure for client connection management
typedef struct {
    pid_t pid;                 // Client PID
    int active;               // Is client active?
    char fifo_in[128];        // FIFO for client->server communication
    char fifo_out[128];       // FIFO for server->client communication
    int fd_in;                // File descriptor for input FIFO
    int fd_out;               // File descriptor for output FIFO
} client_connection_t;

#endif // SERV_CLI_FIFO_H
