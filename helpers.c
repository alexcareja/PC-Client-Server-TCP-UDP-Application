#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include "helpers.h"

cliList *createCliList() {
	// initializeare cliList
	cliList *l = (cliList *) malloc(sizeof(cliList));
	l->cl = NULL;
	l->size = 0;
	return l;
}
int connectClient(cliList **l, int sfd, char *id) {
	/* incearca sa conecteze un client
	Return values: 
		1	- conectat cu succes
		2	- reconectat cu succes
		-1	- utlizatorul cu acest id este deja conectat
	*/
	if ((*l)->cl == NULL) {
		// daca lista de clienti nu a fost initialziazta
		(*l)->cl = (tcpClient *) calloc(MAX_CLIENTS, sizeof(tcpClient));
		(*l)->size = 1;
		(*l)->cl[0].sockfd = sfd;
		strcpy((*l)->cl[0].id, id);
		(*l)->cl[0].status = 1;	// online
		(*l)->cl[0].saved = NULL;
		(*l)->cl[0].no_saved = 0;
		return 1;	// client conectat cu succes
	}
	// verifica daca exista deja un client cu acest nume
	int i, s, d, m;
	s = 0;
	d = (*l)->size - 1;
	// cautare binara dupa id daca exista deja 
	while (s <= d) {
		m = s + (d - s) / 2;
		if (strcmp((*l)->cl[m].id, id) == 0) {
			if ((*l)->cl[m].status) {
				// deja este conectat un client cu acest id
				return -1;
			}
			/* altfel, clientul cu acest id s-a deconectat in trecut si
			acum il reconectez*/
			(*l)->cl[m].status = 1;
			(*l)->cl[m].sockfd = sfd;
			return 2;	// client reconectat cu succes
		}
		if (strcmp((*l)->cl[m].id, id) < 0) {
			s = m + 1;
		} else {
			d = m - 1;
		}
	}
	 /* daca am ajuns aici inseamna ca nu exista un client cu accest id;
	    cl este un vector de clienti sortat crescator dupa id, si la insertia
	    unul element nou, pastrez vectorul sortat*/
	(*l)->size = (*l)->size + 1;
	for (i = (*l)->size - 2; i >= 0; i--) {
		if (strcmp((*l)->cl[i].id, id) < 0) {
			(*l)->cl[i + 1].sockfd = sfd;
			strcpy((*l)->cl[i + 1].id, id);
			(*l)->cl[i + 1].status = 1;
			(*l)->cl[i + 1].saved = NULL;
			(*l)->cl[i + 1].no_saved = 0;
			return 1;
		}
		memcpy(&((*l)->cl[i + 1]), &((*l)->cl[i]), sizeof(tcpClient));
	}
	(*l)->cl[0].sockfd = sfd;
	strcpy((*l)->cl[0].id, id);
	(*l)->cl[0].status = 1;
	(*l)->cl[0].saved = NULL;
	(*l)->cl[0].no_saved = 0;
	return 1;	// client adaugat cu succes
}


char *getClientId(cliList *l, int sfd) {
	// returneaza id ul clientului cu fd dat ca parametru
	int i;
	// cautare liniara, decoarece vectorul e sortat dupa id, nu sockfd
	for (i = 0; i < l->size; i++) {
		if (l->cl[i].sockfd == sfd) {
			return l->cl[i].id;
		}
	}
	return NULL;
}

int getClientFD(cliList *l, char *id) {
	// returneaza fd ul clientului cu id dat ca parametru
	int s, d, m;
	s = 0;
	d = l->size - 1;
	// cautare binara
	while (s <= d) {
		m = s + (d - s) / 2;
		if (strcmp(l->cl[m].id, id) == 0) {
			// daca am gasit clientul cu acest id, returnez fd lui
			return l->cl[m].sockfd;
		}
		if (strcmp(l->cl[m].id, id) < 0) {
			// cauta in jumatatea din dreapta
			s = m + 1;
		} else {
			// cauta in jumatatea din stanga
			d = m - 1;
		}
	}
	// daca nu a gasit un client cu id-ul specificat
	return -1;
}

int isClientOnline(cliList *l, char *id) {
	/*	0 = offline
		1 = online
		-1 = not found*/
	int s, d, m;
	s = 0;
	d = l->size - 1;
	while (s <= d) {
		m = s + (d - s) / 2;
		if (strcmp(l->cl[m].id, id) == 0) {
			// daca am gasit clientul cu acest id, returnez statusul lui
			return l->cl[m].status;
		}
		if (strcmp(l->cl[m].id, id) < 0) {
			// cauta in jumatatea din dreapta
			s = m + 1;
		} else {
			// cauta in jumatatea din stanga
			d = m - 1;
		}
	}
	return -1;
}

void storeMessage(cliList *l, char *id, tcp_message *tm) {
	// retine mesaje pentru un client cu SF activat
	int s, d, m;
	s = 0;
	d = l->size - 1;
	// cautare binara pentru client
	while (s <= d) {
		m = s + (d - s) / 2;
		if (strcmp(l->cl[m].id, id) == 0) {
			// am gasit clientul cautat si adaug mesajul in lista sa de mesaje salvate
			if (l->cl[m].saved == NULL) {
				l->cl[m].saved = (tcp_message *) calloc(MAX_SAVED, sizeof(tcp_message));
				memcpy(&(l->cl[m].saved[0]), tm, sizeof(tcp_message));
				l->cl[m].no_saved = 1;
				return;
			}
			memcpy(&(l->cl[m].saved[l->cl[m].no_saved]), tm, sizeof(tcp_message));
			l->cl[m].no_saved++;
			return;
		}
		if (strcmp(l->cl[m].id, id) < 0) {
			// cauta in dreapta
			s = m + 1;
		} else {
			// cauta in stanga
			d = m - 1;
		}
	}
}


tcp_message *getSaved(cliList *l, char *id) {
	// returneaza lista cu mesaje salvate
	int s, d, m;
	tcp_message *tcpm;
	s = 0;
	d = l->size - 1;
	while (s <= d) {
		m = s + (d - s) / 2;
		if (strcmp(l->cl[m].id, id) == 0) {
			// am gasit clientul cautat si returnez lista sa de mesaje salvate
			tcpm = l->cl[m].saved;
			l->cl[m].saved = NULL;
			return tcpm;
		}
		if (strcmp(l->cl[m].id, id) < 0) {
			// cauta in dreapta
			s = m + 1;
		} else {
			// cauta in stanga
			d = m - 1;
		}
	}
	return NULL;
}
int getSavedCount(cliList *l, char *id) {
	// returneaza numarul de mesaje salvate
	int s, d, m, x;
	s = 0;
	d = l->size - 1;
	while (s <= d) {
		m = s + (d - s) / 2;
		if (strcmp(l->cl[m].id, id) == 0) {
			// am gasit clientul cautat si returnez numarul de mesaje salvate
			x = l->cl[m].no_saved;
			l->cl[m].no_saved = 0;
			return x;
		}
		if (strcmp(l->cl[m].id, id) < 0) {
			// cauta in dreapta
			s = m + 1;
		} else {
			// cauta in stanga
			d = m - 1;
		}
	}
	return 0;
}

void logoutClient(cliList *l, int sfd) {
	// deconecteaza client din lista
	int i;
	for (i = 0; i < l->size; i++) {
		if (l->cl[i].sockfd == sfd) {
			l->cl[i].status = 0;	// clientul s-a deconectat
			l->cl[i].sockfd = -1;	// nu trebuie retinut fostul fd
			return;
		}
	}
}

void destroyCliList(cliList *l) {
	// eliberare memorie alocata 
	if (l == NULL) {
		return;
	}
	int i;
	if (l->cl != NULL) {
		for (i = 0; i < l->size; i++) {
			if (l->cl[i].saved != NULL) {
				free(l->cl[i].saved);
			}
		}
		free(l->cl);
	}
	free(l);
}

topicDB *initTDB() {
	// initializare baza de date de topicuri
	topicDB *tdb = (topicDB *) malloc(sizeof(topicDB));
	tdb->tlist = (topic *) calloc(MAX_TOPICS, sizeof(topic));
	tdb->size = 0;
	return tdb;
}

int subscribe(topicDB *tdb, char *tname, char *id, int SF) {
	/* aboneaza un client la un topic
	   Return values:
	   		1 = succes
	   		-1 = fail  */
	if (tdb->size == 0) {
		// daca nu exista niciun topic inca
		tdb->size = 1;
		strcpy(tdb->tlist[0].name, tname);
		tdb->tlist[0].subscribers = (follower *) calloc(MAX_CLIENTS, sizeof(follower));
		strcpy(tdb->tlist[0].subscribers[0].id, id);
		tdb->tlist[0].subscribers[0].SF = SF;
		tdb->tlist[0].count = 1;;
		return 1;
	}
	int i, s, d, m;
	s = 0;
	d = tdb->size - 1;
	// cautare binara pentru a gasit topicul
	while (s <= d) {
		m = s + (d - s) / 2;
		if (strcmp(tdb->tlist[m].name, tname) == 0) {
			for (i = 0; i < tdb->tlist[m].count; i++) {
				if (strcmp(tdb->tlist[m].subscribers[i].id, id) == 0) {
					return -1; // utilizator deja abonat
				}
			}
			strcpy(tdb->tlist[m].subscribers[tdb->tlist[m].count].id, id);
			tdb->tlist[m].subscribers[tdb->tlist[m].count].SF = SF;
			tdb->tlist[m].count++;
			return 1;	// client abonat cu succes
		}
		if (strcmp(tdb->tlist[m].name, tname) < 0) {
			// cauta in dreapta
			s = m + 1;
		} else {
			// cauta in stanga
			d = m - 1;
		}
	}
	// Ajung aici daca nu exista topicul respectiv, si trebuie creat.
	// Pastreaza ordinea crescatoare in functie de topic in lista de topicuri
	tdb->size++;
	for (i = tdb->size - 2; i >= 0; i--) {
		if (strcmp(tdb->tlist[i].name, tname) < 0) {
			tdb->tlist[i + 1].count = 1;
			strcpy(tdb->tlist[i + 1].name, tname);
			tdb->tlist[i + 1].subscribers = (follower *) calloc(MAX_CLIENTS,
				sizeof(follower));
			strcpy(tdb->tlist[i + 1].subscribers[0].id, id);
			tdb->tlist[i + 1].subscribers[0].SF = SF;
			tdb->tlist[i + 1].count = 1;;
			return 1;	// client adaugat cu succes
		}
		memcpy(&(tdb->tlist[i + 1]), &(tdb->tlist[i]), sizeof(topic));
	}
	// daca trebuie adaugat ca primul element
	tdb->tlist[0].count = 1;
	strcpy(tdb->tlist[0].name, tname);
	tdb->tlist[0].subscribers = (follower *) calloc(MAX_CLIENTS,
		sizeof(follower));
	strcpy(tdb->tlist[0].subscribers[0].id, id);
	tdb->tlist[0].subscribers[0].SF = SF;
	tdb->tlist[0].count = 1;;
	return 1;// client abonat cu succes
}
int unsubscribe(topicDB *tdb, char *tname, char *id) {
	/* dezaboneaza un utilizator de la un topic
	   Return values:
	   		1 = succes
	   		-1 = fail  */
	int i, s, d, m;
	s = 0;
	d = tdb->size - 1;
	// cautare binara dupa numele topicului
	while (s <= d) {
		m = s + (d - s) / 2;
		if (strcmp(tdb->tlist[m].name, tname) == 0) {
			// daca am gasit topicul
			for (i = 0; i < tdb->tlist[m].count; i++) {
				if (strcmp(tdb->tlist[m].subscribers[i].id, id) == 0) {
					while (i < tdb->tlist[m].count - 1) {
						memcpy(&(tdb->tlist[m].subscribers[i]),
							&(tdb->tlist[m].subscribers[i + 1]), sizeof(follower));
						i++;
					}
					memset(&(tdb->tlist[m].subscribers[i]), 0 , sizeof(follower));
					tdb->tlist[m].count--;
					return 1; // dezabonare cu succes
				}
			}
			break;
		}
		if (strcmp(tdb->tlist[m].name, tname) < 0) {
			// cauta in dreapta
			s = m + 1;
		} else {
			// cauta in stanga
			d = m - 1;
		}
	}
	return -1; // nu a fost gasit utilizatorul ca abonat al topicului specificat
}

follower *getSubscribers(topicDB *tdb, char *tname) {
	// returneaza lista de urmaritori a unui topic sau NULL
	int s, d, m;
	s = 0;
	d = tdb->size - 1;
	while (s <= d) {
		m = s + (d - s) / 2;
		if (strcmp(tdb->tlist[m].name, tname) == 0) {
			// daca am gasit topicul
			return tdb->tlist[m].subscribers;
		}
		if (strcmp(tdb->tlist[m].name, tname) < 0) {
			// cauta in dreapta
			s = m + 1;
		} else {
			// cauta in stanga
			d = m - 1;
		}
	}
	// daca nu a fost gasit topicul
	return NULL;
}

int getSubscribersCount(topicDB *tdb, char *tname) {
	// returneaza numarul de urmaritori ai unui topic
	int s, d, m;
	s = 0;
	d = tdb->size - 1;
	while (s <= d) {
		m = s + (d - s) / 2;
		if (strcmp(tdb->tlist[m].name, tname) == 0) {
			return tdb->tlist[m].count;
		}
		if (strcmp(tdb->tlist[m].name, tname) < 0) {
			// cauta in dreapta
			s = m + 1;
		} else {
			// cauta in stanga
			d = m - 1;
		}
	}
	return -1;
}


void destroyTDB(topicDB *tdb) {
	// elibereaza memoria alocata
	if (tdb == NULL) {
		return;
	}
	int i;
	for (i = 0; i < tdb->size; i++) {
		if(tdb->tlist[i].subscribers != NULL) {
			free(tdb->tlist[i].subscribers);
		}
	}
	free(tdb->tlist);
	free(tdb);
}

void printMessage (tcp_message m) {
	// formatare si afisare mesaj
	printf("%s:%d - %s - ", inet_ntoa(m.header.udp_client.sin_addr), 
		ntohs(m.header.udp_client.sin_port), m.topic);
	switch(m.data_type) {
		case TSTRING:
			// mesaj tip string
			printf("STRING - %s\n", m.payload);
			break;

		case TFLOAT:
			// mesaj tip float
			printf("FLOAT - ");
			char semn = 0;
			uint32_t real = 0;
			uint8_t p = 0;
			unsigned int ten = 1;
			memcpy(&semn, &(m.payload), 1);
			memcpy(&real, &(m.payload[1]),sizeof(uint32_t));
			memcpy(&p, &(m.payload[5]), sizeof(uint8_t));
			real = ntohl(real);
			if (semn) {
				semn = 45;	// cod ascii penru minus
			}
			while (p) {	// creeaza puterea lui 10
				ten *= 10;
				p--;
			}
			printf("%c", semn);
			if (real / ten != 0) {
				printf("%u.%u", real / ten, real % ten);
			} else {
				printf("0.");
				while(ten) {
					ten = ten / 10;
					if (real / ten == 0) {
						printf("0");
					} else {
						printf("%u", real);
						break;
					}
				}
			}
			printf("\n");
			break;

		case TSHORT_REAL:
			// mesaj tip short_real
			printf("SHORT_REAL - ");
			uint16_t num;
			memcpy(&num, &(m.payload), sizeof(uint16_t));
			num = ntohs(num);
			printf("%d.%d%d\n", num / 100, (num % 100) / 10, (num % 100) % 10);
			break;

		case TINT:
			// mesaj tip int
			printf("INT - ");
			char semn_int = 0;
			if (m.payload[0]) {
				semn_int = 45;
			}
			uint32_t n;
			memcpy(&n, &(m.payload[1]), sizeof(uint32_t));
			n = ntohl(n);
			printf("%c%d\n", semn_int, n);
			break;

		default:
			break;
	}
}
