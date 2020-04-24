#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

int main(int argc, char const *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	char read_buffer[65];
	char *token;
	message m;
	subscribe_message sm;

	if (argc < 4) {
		perror("Wrong usage");
		return -1;
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	strcpy(buffer, argv[1]);
	n = send(sockfd, buffer, BUFLEN, 0);
	DIE(n < 0, "send");

	fd_set fdset;
	fd_set tempfds;
	FD_SET(sockfd, &fdset);
	FD_SET(0, &fdset);
	int fdmax = sockfd;

	while (1) {
		tempfds = fdset;
		ret = select(fdmax + 1, &tempfds, NULL, NULL, NULL);
		if (ret < 0) {
			perror("Eroare select");
		}
		if (FD_ISSET(0, &tempfds)) {
			// se citeste de la tastatura
			memset(read_buffer, 0, sizeof(read_buffer));
			fgets(read_buffer, sizeof(read_buffer), stdin);
			if (strncmp(read_buffer, "exit", 4) == 0) { // exit -> deconectare
				break;
			}
			memset(&sm, 0, sizeof(subscribe_message));
			token = strtok(read_buffer, " ");
			if (strcmp(token, "subscribe") == 0) {
				sm.subscribe = 1;
				// seteaza topicul la care se aboneaza utilizatorul
				token = strtok(NULL, " ");
				strcpy(sm.topic, token);
				// seteaza parametrul SF
				token = strtok(NULL, " ");
				sm.SF = atoi(token);
			} else {
				if (strcmp(token, "unsubscribe") == 0) {
					sm.subscribe = 0;
					// seteaza topicul de la care se dezaboneaza utilizatorul
					token = strtok(NULL, " ");
					strcpy(sm.topic, token);
				} else {
					DIE(1, "input");
				}
			}
			// se trimite mesaj la server
			n = send(sockfd, &sm, sizeof(sm), 0);
			DIE(n < 0, "send");
			memset(&sm, 0, sizeof(sm));
		}
		else {
			// asteapta mesaj de la server
			n = recv(sockfd, &m, sizeof(m), 0);
			if (n == 0) {
				break;
			}
        	//fprintf(stderr, "Received: %s", buffer);
			memset(buffer, 0, BUFLEN);
		}
		
	}

	close(sockfd);
	return 0;
}