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
#include <string.h>
#include "serv_cli_fifo.h"
#include "Handlers_Serv.h"
#include <sys/select.h>

client_connection_t clients[MAX_CLIENTS];
volatile sig_atomic_t nb_clients = 0;

void cleanup_client(int idx) {
    if (clients[idx].active) {
        close(clients[idx].fd_in);
        close(clients[idx].fd_out);
        unlink(clients[idx].fifo_in);
        unlink(clients[idx].fifo_out);
        clients[idx].active = 0;
        clients[idx].pid = 0;
        nb_clients--;
        printf("Server: Client %d cleaned up\n", clients[idx].pid);
    }
}

void handle_client(int client_idx) {
    question_t question;
    reponse_t reponse;

    // Read request
    ssize_t bytes_read = read(clients[client_idx].fd_in, &question, sizeof(question_t));
    if (bytes_read <= 0) {
        if (bytes_read == 0 || errno != EINTR) {
            printf("Server: Client %d disconnected\n", clients[client_idx].pid);
            cleanup_client(client_idx);
        }
        return;
    }

    printf("Server [%d]: Processing request from client %d (requesting %d numbers)\n",
           getpid(), question.client_id, question.n);

    // Prepare response
    reponse.client_id = getpid();
    reponse.n = question.n;
    for (int i = 0; i < question.n; i++) {
        reponse.nombres[i] = rand() % 100;
    }

    // Send response
    if (write(clients[client_idx].fd_out, &reponse, sizeof(reponse_t)) == -1) {
        perror("Server: Error writing response");
        return;
    }

    // Signal client
    if (kill(clients[client_idx].pid, SIGUSR1) == -1) {
        perror("Server: Error sending signal");
        return;
    }
}

int main() {
    int registry_fd;
    struct sigaction sa;

    // Initialize client array
    memset(clients, 0, sizeof(clients));

    // Setup signal handlers
    sa.sa_handler = hand_reveil;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("Server: Error setting up SIGUSR1 handler");
        exit(1);
    }

    sa.sa_handler = fin_serveur;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Server: Error setting up SIGINT handler");
        exit(1);
    }

    // Clean up existing FIFOs before starting
    printf("Server: Cleaning up existing FIFOs...\n");

    unlink(SERVER_REGISTRY); // Ensure the registry FIFO is removed

   // Create server registry FIFO
   printf("Server: Creating registry FIFO...\n");

   if (mkfifo(SERVER_REGISTRY, 0666) == -1) {
       perror("Server: Error creating registry FIFO");
       exit(1);
   }

   printf("Server [%d]: Starting...\n", getpid());

   // Open registry FIFO
   registry_fd = open(SERVER_REGISTRY, O_RDONLY | O_NONBLOCK);

   if (registry_fd == -1) {
       perror("Server: Error opening registry FIFO");
       exit(1);
   }

   // Initialize random number generator
   srand(time(NULL));

   // Main server loop
   while (1) { // Run indefinitely until interrupted
       fd_set readfds;
       struct timeval tv = {1, 0};  // 1 second timeout

       FD_ZERO(&readfds);
       FD_SET(registry_fd, &readfds);

       int ready = select(registry_fd + 1, &readfds, NULL, NULL, &tv);
       if (ready == -1 && errno != EINTR) {
           perror("Server: Select error");
           continue;
       }

       // Check for new client registrations
       if (FD_ISSET(registry_fd, &readfds)) {
           pid_t new_client_pid;
           if (read(registry_fd, &new_client_pid, sizeof(pid_t)) > 0) {
               if (nb_clients < MAX_CLIENTS) {
                   // Find empty slot for new client
                   int idx = 0;
                   while (idx < MAX_CLIENTS && clients[idx].active) idx++;

                   if (idx < MAX_CLIENTS) {
                       // Setup client FIFOs
                       snprintf(clients[idx].fifo_in, sizeof(clients[idx].fifo_in),
                                "%s%d_in", FIFO_BASE, new_client_pid);
                       snprintf(clients[idx].fifo_out, sizeof(clients[idx].fifo_out),
                                "%s%d_out", FIFO_BASE, new_client_pid);

                       printf("Server: Creating FIFOs for client PID %d...\n", new_client_pid);

                       // Create input FIFO and check for errors
                       if (mkfifo(clients[idx].fifo_in, 0666) == -1 && errno != EEXIST) {
                           perror("Server: Error creating input FIFO");
                           continue; 
                       }

                       // Create output FIFO and check for errors
                       if (mkfifo(clients[idx].fifo_out, 0666) == -1 && errno != EEXIST) {
                           perror("Server: Error creating output FIFO");
                           unlink(clients[idx].fifo_in); 
                           continue; 
                       }

                       // Open FIFOs in non-blocking mode
                       clients[idx].fd_in = open(clients[idx].fifo_in, O_RDONLY | O_NONBLOCK);
                       clients[idx].fd_out = open(clients[idx].fifo_out, O_WRONLY | O_NONBLOCK);

                       if (clients[idx].fd_in != -1 && clients[idx].fd_out != -1) {
                           clients[idx].pid = new_client_pid;
                           clients[idx].active = 1;
                           nb_clients++;
                           printf("Server: New client connected (PID: %d)\n", new_client_pid);
                       } else {
                           perror("Server: Error opening FIFOs after creation");
                           unlink(clients[idx].fifo_in); 
                           unlink(clients[idx].fifo_out); 
                       }
                   }
               }
           }
       }

       // Handle client requests
       for (int i = 0; i < MAX_CLIENTS; i++) {
           if (clients[i].active && FD_ISSET(clients[i].fd_in, &readfds)) {
               handle_client(i);
           }
       }
   }

   // Cleanup on exit
   printf("Server: Shutting down...\n");
   for (int i = 0; i < MAX_CLIENTS; i++) {
       cleanup_client(i);
   }
   close(registry_fd);
   unlink(SERVER_REGISTRY);

   return 0;
}