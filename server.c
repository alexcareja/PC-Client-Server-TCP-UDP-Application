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
	int i, j, n, ret;
	int sockfd, udp_sockfd, newsockfd, port_number;
	char buffer[BUFLEN];
	message m;
	subscribe_message sm;
	char **client = (char **) malloc(MAX_CLIENTS * sizeof(char *));
	struct sockaddr_in serv_addr, client_addr;
	socklen_t clilen;

	DIE(argc < 2, "usage");

	fd_set read_fds;	// multimea de citire
	fd_set tcp_fds;		// multimea de fd tcp
	fd_set udp_fds;		// multimea de fd udp
	fd_set tmp_fds;		// multimea temporara
	int fdmax;			// valoare maxima fd in read_fds

	FD_ZERO(&read_fds);
	FD_ZERO(&tcp_fds);
	FD_ZERO(&udp_fds);
	FD_ZERO(&tmp_fds);

	port_number = atoi(argv[1]);
	DIE(port_number == 0, "atoi");

	// Deschidere socket UDP
	udp_sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(udp_sockfd < 0, "socket");
	// seteaza structura sockaddr_in
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port_number);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	// leaga socketul UDP
	ret = bind(udp_sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");
	// adauga noul file descriptor (socketul UDP) in multimea read_fds
	FD_SET(udp_sockfd, &read_fds);
	fdmax = udp_sockfd;

	// creare socket TCP pentru ascultat
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");
	// leaga socketul TCP
	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");
	// marcheaza socketul TCP ca unul pasiv
	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");
	// adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = sockfd;

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		if (FD_ISSET(0, &tmp_fds)) {
			// verifica daca primeste mesaj exit
			memset(buffer, 0, BUFLEN);
			fgets(buffer, sizeof(buffer), stdin);
			if (strncmp(buffer, "exit", 4) == 0) { // exit -> deconectare
				for (i = 0; i <= fdmax; i++) {
					if (FD_ISSET(i, &read_fds)) {
						close(i);
					}
				}
				break;
			}
		}
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// a venit o cerere de conexiune pe socketul TCP inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(client_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &client_addr, &clilen);
					DIE(newsockfd < 0, "accept");
					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					FD_SET(newsockfd, &tcp_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					client[newsockfd] = (char *) calloc(BUFLEN, sizeof(char));
					n = recv(newsockfd, client[newsockfd], BUFLEN, 0);
					DIE(n < 0, "recv");
					printf("New client %s connected from %s:%d.\n", client[newsockfd],
							inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
					continue;
				} 
				if (i == udp_sockfd) {
					// a venit o cerere de conexiune pentru UDP pe care serverul o accepta
					clilen = sizeof(client_addr);
					newsockfd = accept(udp_sockfd, (struct sockaddr *) &client_addr, &clilen);
					DIE(newsockfd < 0, "accept");
					FD_SET(newsockfd, &read_fds);
					FD_SET(newsockfd, &udp_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
				} else {
					if (FD_ISSET(i, &tcp_fds)) {
						// s-au primit date pe unul din socketii de client TCP
						// asa ca serverul trebuie sa le receptioneze
						memset(&sm, 0, sizeof(sm));
						n = recv(i, &sm, sizeof(sm), 0);
						DIE(n < 0, "recv");

						if (n == 0) {
							// conexiunea s-a inchis
							printf("Client %s disconnected.\n", client[i]);
							close(i);
							// se scoate din multimea de citire socketul inchis 
							FD_CLR(i, &read_fds);
							FD_CLR(i, &tcp_fds);
						} else {
							// am primit un mesaj de tip subscribe/unsubscribe de la un utilizator
							DIE(n < 0, "send");
							printf ("S-a primit de la client de pe socketul %d mesajul\n", i);
						}

					} else {
						if (FD_ISSET(i, &udp_fds)) {
							// s-au primit date de pe unul din socketii UDP
							// si serverul va redirectiona datele catre abonatii 
							n = recvfrom(udp_sockfd, &m, sizeof(m), 0,
								(struct sockaddr *) &client_addr, &clilen);
							DIE(n < 0, "recvfrom");
						}
					}
				}
			}
		}
	}

	return 0;
}