# Makefile pour le projet client-serveur utilisant FIFO

# Variables
CC = gcc
CFLAGS = -Wall -Wextra -g
OBJ = Client.o Serveur.o  # Changer 'client.o' en 'Client.o' et 'serveur.o' en 'Serveur.o'
EXEC_CLIENT = Client
EXEC_SERVEUR = Serveur

# Règles par défaut
all: $(EXEC_CLIENT) $(EXEC_SERVEUR)

# Règle pour compiler le client
$(EXEC_CLIENT): Client.o
	$(CC) $(CFLAGS) -o $@ $^

# Règle pour compiler le serveur
$(EXEC_SERVEUR): Serveur.o
	$(CC) $(CFLAGS) -o $@ $^

# Règle pour compiler les fichiers objets
%.o: %.c serv_cli_fifo.h Handlers_Serv.h
	$(CC) $(CFLAGS) -c $<

# Règle pour nettoyer les fichiers générés
clean:
	rm -f $(OBJ) $(EXEC_CLIENT) $(EXEC_SERVEUR)
	rm -f /tmp/fifo_client_*_in /tmp/fifo_client_*_out /tmp/server_registry


# Règle pour exécuter le serveur et le client (exécution séquentielle)
run: $(EXEC_SERVEUR) $(EXEC_CLIENT)
	@echo "Démarrage du serveur..."
	./$(EXEC_SERVEUR) &
	sleep 1  # Attendre que le serveur soit prêt
	@echo "Démarrage du client..."
	./$(EXEC_CLIENT)

.PHONY: all clean run