#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"

int main(int argc, char const *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	char read_buffer[65];
	char feedback_message[65];
	char *token;
	subscribe_message sm;
	tcp_message m;

	if (argc < 4) {
		perror("Wrong usage");
		return -1;
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	int disable = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &disable, sizeof(int)); // disable Neagle algorithm

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");

	strcpy(buffer, argv[1]);
	n = send(sockfd, buffer, BUFLEN, 0);
	DIE(n < 0, "send");
	memset(buffer, 0, BUFLEN);
	n = recv(sockfd, buffer, BUFLEN, 0);
	DIE(n < 0, "recv");
	if (strcmp(buffer, "existing") == 0) {
		printf("Acest ID de utilizator este deja folosit.\n");
		close(sockfd);
		return -1;
	}

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
			memset(&feedback_message, 0, sizeof(feedback_message));
			token = strtok(read_buffer, " ");
			strcpy(feedback_message, token);
			strcat(feedback_message, "d ");
			if (strcmp(token, "subscribe") == 0) {
				sm.subscribe = 1;
				// seteaza topicul la care se aboneaza utilizatorul
				token = strtok(NULL, " \t\r\n");
				strcpy(sm.topic, token);
				// seteaza parametrul SF
				token = strtok(NULL, " \t\r\n");
				if (token == NULL) {
					printf("Parametrul SF nu a fost setat.\n");
					continue;
				}
				if (token[0] < 48 || token[0] > 49) {
					printf("Parametrul SF poate lua valorile 0/1.\n");
					continue;
				}
				int sf = atoi(token);
				sm.SF = sf;
			} else {
				if (strcmp(token, "unsubscribe") == 0) {
					sm.subscribe = 0;
					// seteaza topicul de la care se dezaboneaza utilizatorul
					token = strtok(NULL, " \t\r\n");
					strcpy(sm.topic, token);
				} else {	// altceva in afara de subscribe/unsubscribe/exit
					printf("Nu recunosc comanda '%s'.\n", token);
					continue;
				}
			}
			// se trimite mesaj la server
			n = send(sockfd, &sm, sizeof(sm), 0);
			DIE(n < 0, "send");
			memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, BUFLEN, 0);
			// verifica mesajul de la server
			if (strcmp(buffer, "ok") == 0) {	// actiunea s-a realizat cu succes
				strcat(feedback_message, sm.topic);
				printf("%s\n", feedback_message);
			} else {
				if (strcmp(buffer, "ex") == 0) {	// deja este abonat
					printf("Esti deja abonat la acest canal. Actiunea a fost anulata.\n");
				} else {
					if (strcmp(buffer, "nex") == 0) {	// nu este abonat
						printf("Nu te poti dezabona de la un topic la care nu esti abonat.\n");
					}
				}
			}
			memset(&sm, 0, sizeof(sm));
		}
		else {
			// asteapta mesaj de la server
			memset(&m, 0, sizeof(tcp_message));
			// primeste headerul
			n = recv(sockfd, &(m.header), sizeof(tcp_header), 0);
			DIE(n < 0, "recv");
			if (n == 0) {
				break;
			}
			// primeste restul mesajului
			n = recv(sockfd, &(m.topic), m.header.len, 0);
			DIE(n < 0, "recv");
			// apel functie de parsat mesaj
			printMessage(m);
			memset(buffer, 0, BUFLEN);
		}
	}
	// inchide socketul TCP
	close(sockfd);
	return 0;
}
