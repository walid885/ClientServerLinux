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

// Global variable for signal handling
volatile sig_atomic_t signal_recu = 0;

// Signal handler for SIGUSR1
void handle_sigusr1(int signo) {
    if (signo == SIGUSR1) {
        printf("[CLIENT %d][%ld] Signal SIGUSR1 received\n", getpid(), time(NULL));
        signal_recu = 1;
    }
}

// Function to print the current timestamp
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

    pid_t pid = getpid();

    // Configuration of signal handler
    struct sigaction sa;
    sa.sa_handler = handle_sigusr1;
    sa.sa_flags = 0;

	sigemptyset(&sa.sa_mask);

	if(sigaction(SIGUSR1,&sa,NULL)==-1){
		print_timestamp();
		perror("[CLIENT] Error configuring signal SIGUSR1");
		exit(EXIT_FAILURE);
	}

	print_timestamp();
	printf("[CLIENT %d] Starting client\n", pid);

	// Open the registry FIFO for registration with the server.
	print_timestamp();
	printf("[CLIENT %d] Attempting to open registry (%s)\n", pid,SERVER_REGISTRY);

	registry_fd=open(SERVER_REGISTRY,O_WRONLY);

	if(registry_fd==-1){
		print_timestamp();
		perror("[CLIENT] Failed to open registry");
		exit(EXIT_FAILURE);	
	}

	// Send PID to the server via the registry.
	if(write(registry_fd,&pid,sizeof(pid_t))==-1){
		print_timestamp();
		perror("[CLIENT] Failed to send PID to registry");
		close(registry_fd);
		exit(EXIT_FAILURE);	
	}
	
	close(registry_fd);

	char fifo_in[128], fifo_out[128];

	snprintf(fifo_in,sizeof(fifo_in),"%s%d_in",FIFO_BASE,pid); 
	snprintf(fifo_out,sizeof(fifo_out),"%s%d_out",FIFO_BASE,pid); 

	int attempts=0;

	while(attempts<10){ 
		fd_in=open(fifo_in,O_RDONLY|O_NONBLOCK); 
		if(fd_in!=-1){ 
			break; 
		}
		
		print_timestamp();
		printf("[CLIENT %d] Waiting for FIFO in (%s)...\n",pid,fifo_in); 
		sleep(1); 
		attempts++; 
	}

	if(fd_in==-1){ 
		print_timestamp(); 
		perror("[CLIENT] Failed to open FIFO in after waiting"); 
		exit(EXIT_FAILURE); 
	} 

	print_timestamp(); 
	printf("[CLIENT %d] Successfully opened FIFO in (%s)\n",pid,fifo_in); 

	attempts=0; 

	while(attempts<10){ 
		fd_out=open(fifo_out,O_WRONLY|O_NONBLOCK); 
		if(fd_out!=-1){ 
			break; 
		}
		
		print_timestamp(); 
		printf("[CLIENT %d] Waiting for FIFO out (%s)...\n",pid,fifo_out); 
		sleep(1); 
		attempts++; 
	}

	if(fd_out==-1){ 
	    print_timestamp(); 
	    perror("[CLIENT] Failed to open FIFO out after waiting"); 
	    close(fd_in); exit(EXIT_FAILURE); 
	  } 

	print_timestamp(); 
	printf("[CLIENT %d] Successfully opened FIFO out (%s)\n",pid,fifo_out); 

	srand(time(NULL)+pid);

	question.client_id=pid; 
	question.n=rand()%NMAX+1;

	print_timestamp(); 
	printf("[CLIENT %d] Preparing request: %d random numbers\n",question.client_id,question.n);

	ssize_t write_result=write(fd_out,&question,sizeof(question_t)); 

	if(write_result==-1){ 
	    print_timestamp(); 
	    perror("[CLIENT] Error sending question"); 
	    close(fd_in); close(fd_out); exit(EXIT_FAILURE);  
	  } 

	print_timestamp(); printf("[CLIENT %d] Question sent successfully (%zd bytes)\n",question.client_id,write_result);

	print_timestamp(); printf("[CLIENT %d] Waiting for signal from server...\n",question.client_id);

	while(!signal_recu){ pause(); } 

	print_timestamp(); printf("[CLIENT %d] Attempting to read response\n",question.client_id);

	ssize_t read_result=read(fd_in,&reponse,sizeof(reponse_t)); 

	if(read_result==-1){ print_timestamp(); perror("[CLIENT] Error reading response"); close(fd_in); close(fd_out); exit(EXIT_FAILURE);} 

	print_timestamp(); printf("[CLIENT %d] Response received (%zd bytes)\n",question.client_id,read_result);

	print_timestamp(); printf("[CLIENT %d] Numbers received (%d): ",reponse.client_id,reponse.n); 

	for(int i=0;i<reponse.n;i++){ printf("%d ",reponse.nombres[i]); } 

	printf("\n");

	print_timestamp(); printf("[CLIENT %d] Sending confirmation signal to server\n",question.client_id);

	if(kill(reponse.client_id,SIGUSR1)==-1){ print_timestamp(); perror("[CLIENT] Error sending confirmation signal"); } else { print_timestamp(); printf("[CLIENT %d] Confirmation signal sent successfully\n",question.client_id);} 

	print_timestamp(); printf("[CLIENT %d] Closing FIFOs\n",question.client_id); close(fd_in); close(fd_out);

	print_timestamp(); printf("[CLIENT %d] Terminating client\n",question.client_id);

	return EXIT_SUCCESS;  
}