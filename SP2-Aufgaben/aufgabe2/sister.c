#include <stdio.h>
#include <stdlib.h>
#include "cmdline.h"
#include "connection.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

static void die(char *message){
    perror(message);
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]){
    
    if(argc != 2 && argc != 3){
        fprintf(stderr, "Usage: ./siter --wwwpath=<dir> [--port=<p>]\n");
        exit(EXIT_FAILURE);
    }

    // Header Dateien initialisieren
    if(cmdlineInit(argc, argv) == -1) die("cmdlineInit");

    if(initConnectionHandler() == -1){
        fprintf(stderr, "Usage: ./sister --wwwpath=<dir> [--port=<p>]\n");
        exit(EXIT_FAILURE);
    }

    // den port aus den Argumenten entlesen
    long port;
    const char *given_port = cmdlineGetValueForKey("port");
    char *endptr;

    if(given_port == NULL){
        port = 1337; // Standard-Port 
    } else {
        errno = 0;
        port = strtol(given_port, &endptr, 10); // convert string to base 10 int
        if (errno){
            die("strtol");
        }
	if(*endptr != '\0'){ // Buchstanben inhalten
		fprintf(stderr, "--port is not a valid number\n");
		exit(EXIT_FAILURE);
	}
        if(port == LONG_MIN || port < 0){
            fprintf(stderr, "--port is too small\n");
            exit(EXIT_FAILURE);
        }
	if(port == LONG_MAX || port > 65535){
            fprintf(stderr, "--port is too large\n");
            exit(EXIT_FAILURE);
        }
    }

    // TCP/IPv6-Socket erzeugen -> auf Anfrage warten
    int serverSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if(serverSocket == -1) die("socket");

    struct sockaddr_in6 addr = {
        .sin6_family = AF_INET6,
        .sin6_port = htons(port),
        .sin6_addr = in6addr_any,
    };

    // damit man den gleichen Port öfter hintereinander benutzen kann
    int flag = 1;
    if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) == -1){
        die("sockopt");
    }
    if(bind(serverSocket, (struct sockaddr *) &addr, sizeof(addr)) == -1){
        die("bind");
    }

    if(listen(serverSocket, SOMAXCONN) == -1) die("listen");

    // angefragte Verbindungen annehmen
    for(;;){
        int clientSocket = accept(serverSocket, NULL, NULL);
        if(clientSocket == -1){
            perror("accept");
            continue;
        }
        handleConnection(clientSocket, serverSocket);
        if(close(clientSocket) == -1) die("close");
    }

    close(serverSocket);

    exit(EXIT_SUCCESS);
}
