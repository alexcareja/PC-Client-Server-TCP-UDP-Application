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
#include <stdint.h>

int main(int argc, char const *argv[])
{
	int i, j, n, ret, size;
	int sockfd, udp_sockfd, newsockfd, port_number;
	char buffer[BUFLEN];
	udp_message m;
	subscribe_message sm;
	tcp_message tm;
	struct sockaddr_in serv_addr, client_addr;
	socklen_t clilen;
	cliList *clients = createCliList();
	topicDB *tdb = initTDB();
	DIE(argc < 2, "usage");

	fd_set read_fds;	// multimea de citire
	fd_set tmp_fds;		// multimea temporara
	int fdmax;			// valoare maxima fd in read_fds

	// initializare cu nultimea vida
	FD_ZERO(&read_fds);
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
	int disable = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &disable, sizeof(int)); // disable Neagle algorithm
	// leaga socketul TCP
	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");
	// marcheaza socketul TCP ca unul pasiv
	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");
	// adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	FD_SET(0, &read_fds);	// file descriptor stdin
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
				for (i = 0; i <= fdmax; i++) {	// inchide socketii deschisi
					if (FD_ISSET(i, &read_fds)) {
						close(i);
					}
				}
				break;
			}
			// daca s-a introdus alta comanda in afara de exit
			printf("Pentru a inchide serverul, introduceti comanda 'exit'\n");
			continue;
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
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					// asteapta mesajul cu id-ul clientului
					memset(buffer, 0, BUFLEN);
					n = recv(newsockfd, buffer, BUFLEN, 0);
					DIE(n < 0, "recv");
					// adauga clientul in lita de clienti
					ret = connectClient(&clients, newsockfd, buffer);
					if (ret >= 0) {
						printf("New client %s connected from %s:%d.\n", buffer,
							inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
						strcpy(buffer, "ok");
						n = send(newsockfd, buffer, BUFLEN, 0);
						DIE(n < 0, "send");
						if (ret == 2) {
							// Clientul s-a reconectat si ii trimit toate mesajele salvate pentru el
							// obtine mesajele in asteptare ale clientului
							tcp_message *mlist = getSaved(clients, getClientId(clients, newsockfd));
							int count = getSavedCount(clients, getClientId(clients, newsockfd));
							for (j = 0; j < count; j++) {
								memset(&tm, 0, sizeof(tcp_message));
								memcpy(&(tm), &(mlist[j]), sizeof(tcp_message));
								size = tm.header.len + sizeof(struct sockaddr_in) + sizeof(int);
								n = send(newsockfd, &tm, size, 0);
								DIE(n < 0, "send");
							}
							if (mlist != NULL) {
								// elibereaza memoria anterior alocata pentru mesajele on hold
								free(mlist);
							}
						}
						
					} else {
						// daca a returnat < 0, atunci exista deja acest id, iar
						// utilizatorul respectiv este conectat
						strcpy(buffer, "existing");
						n = send(newsockfd, buffer, BUFLEN, 0);
						DIE(n < 0, "send");
						FD_CLR(newsockfd, &read_fds);
					}
					continue;
				} 
				if (i == udp_sockfd) {
					// a venit o cerere de conexiune pentru UDP pe care serverul o accepta
					// s-au primit date de pe unul din socketii UDP
					// si serverul va redirectiona datele catre abonatii
					clilen = sizeof(client_addr);
					memset(&m, 0, sizeof(m));
					n = recvfrom(udp_sockfd, &m, sizeof(m), 0,
						(struct sockaddr *) &client_addr, &clilen);
					DIE(n < 0, "recvfrom");
					memset(&tm, 0, sizeof(tcp_message));
					memcpy(&(tm.header.udp_client), &client_addr, clilen);
					// updateaa len in functie de tipul mesajului
					switch(m.data_type) {
						case TSTRING:
							tm.header.len = strlen(m.payload) + TOPIC_LEN + 1;
							break;
						case TFLOAT:
							tm.header.len = FLOAT_LEN + TOPIC_LEN + 1;
							break;
						case TSHORT_REAL:
							tm.header.len = SHORT_REAL_LEN + TOPIC_LEN + 1;
							break;
						case TINT:
							tm.header.len = INT_LEN + TOPIC_LEN + 1;
							break;
						default:
							break;
					}
					// copiaza datele in mesajul nou
					memcpy(&(tm.topic), &(m.topic), TOPIC_LEN);
					tm.data_type = m.data_type;
					memcpy(&(tm.payload), &(m.payload), MAX_PAYLOAD_SIZE);
					size = tm.header.len + sizeof(struct sockaddr_in) + sizeof(int);
					follower *subs = getSubscribers(tdb, m.topic);
					j = getSubscribersCount(tdb, m.topic) - 1;
					for (; j >= 0; j--) {
						if (isClientOnline(clients, subs[j].id)) {
							// daca este online, trimite mesajul acum
							n = send(getClientFD(clients, subs[j].id), &tm, size, 0);
							DIE(n < 0, "send");
						} else {
							// daca nu este online, verifica daca are SF pentru acest topic
							if (subs[j].SF) {
								// salveaza mesajul daca clientul are SF activat pentru acest topic
								storeMessage(clients, subs[j].id, &tm);
							}
						}
					}

				} else {
					if (FD_ISSET(i, &tmp_fds)) {
						// s-au primit date pe unul din socketii de client TCP
						// asa ca serverul trebuie sa le receptioneze
						memset(&sm, 0, sizeof(sm));
						n = recv(i, &sm, sizeof(sm), 0);
						DIE(n < 0, "recv");

						if (n == 0) {
							// conexiunea s-a inchis
							printf("Client %s disconnected.\n", getClientId(clients, i));
							logoutClient(clients, i);
							close(i);
							// se scoate din multimea de citire socketul inchis 
							FD_CLR(i, &read_fds);
						} else {
							// am primit un mesaj de tip subscribe/unsubscribe de la un utilizator
							memset(buffer, 0, BUFLEN);
							strcpy(buffer, "ok");
							if (sm.subscribe) {
								ret = subscribe(tdb, sm.topic, getClientId(clients, i), sm.SF);
								if (ret < 0) {
									strcpy(buffer, "ex");
								}
							} else {
								ret = unsubscribe(tdb, sm.topic, getClientId(clients, i));
								if (ret < 0) {
									strcpy(buffer, "nex");
								}
							}
							// trimite mesaj de feedback cu rezultatul operatiei
							n = send(i, buffer, BUFLEN, 0);
						}
					}
				}
			}
		}
	}
	// elibereaza memoria alocata dinamic
	destroyCliList(clients);
	destroyTDB(tdb);
	return 0;
}
