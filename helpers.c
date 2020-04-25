#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "helpers.h"

subsList *createNode(char *topic) {
	subsList *node = (subsList *) malloc(sizeof(subsList));
	strcpy(node->topic, topic);
	node->next = NULL;
	return node;
}

subsList *removeTopic(subsList *l, char *topic) {
	subsList *aux, *aux2;
	if (strcmp(l->topic, topic) == 0) {
		aux = l->next;
		free(l);
		return aux;
	}
	aux = l;
	while (aux->next != NULL) {
		if (strcmp(aux->next->topic, topic) == 0) {
			aux2 = aux->next;
			aux->next = aux->next->next;
			free(aux2);
			break;
		}
	}
	return l;
}

int findTopic(subsList *l, char *topic) {
	subsList *aux = l;
	while (aux != NULL) {
		if (strcmp(aux->topic, topic) == 0) {
			return 1;
		}
		aux = aux->next;
	}
	return 0;
}

cliList *createCliList() {
	cliList *l = (cliList *) malloc(sizeof(cliList));
	l->cl = NULL;
	l->size = 0;
	//l->capacity = 0;
	return l;
}
int connectClient(cliList **l, int sfd, char *id) {
	if ((*l)->cl == NULL) {
		(*l)->cl = (tcpClient *) calloc(MAX_CLIENTS, sizeof(tcpClient));
		//(*l)->capacity = 2;
		(*l)->size = 1;
		(*l)->cl[0].sockfd = sfd;
		strcpy((*l)->cl[0].id, id);
		(*l)->cl[0].status = 1;	// online
		return 1;	// client conectat cu succes
	}
	// verifica daca exista deja un client cu acest nume
	int i, s, d, m;
	s = 0;
	d = (*l)->size - 1;
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
			return 1;
		}
		memcpy(&((*l)->cl[i + 1]), &((*l)->cl[i]), sizeof(tcpClient));
	}
	(*l)->cl[0].sockfd = sfd;
	strcpy((*l)->cl[0].id, id);
	(*l)->cl[0].status = 1;
	return 1;	// client adaugat cu succes
}


char *getClientId(cliList *l, int sfd) {
	int i;
	for (i = 0; i < l->size; i++) {
		if (l->cl[i].sockfd == sfd) {
			return l->cl[i].id;
		}
	}
	return NULL;
}

int getClientFD(cliList *l, char *id) {
	int s, d, m;
	s = 0;
	d = l->size - 1;
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
	return -1;
}

int isClientOnline(cliList *l, char *id) {
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

void logoutClient(cliList *l, int sfd) {
	int i;
	for (i = 0; i < l->size; i++) {
		if (l->cl[i].sockfd == sfd) {
			l->cl[i].status = 0;	// clientul s-a deconectat
			l->cl[i].sockfd = -1;
			return;
		}
	}
}

void destroyCliList(cliList *l) {
	if (l->cl != NULL) {
		free(l->cl);
	}
	free(l);
}

topicDB *initTDB() {
	topicDB *tdb = (topicDB *) malloc(sizeof(topicDB));
	tdb->tlist = (topic *) calloc(MAX_TOPICS, sizeof(topic));
	tdb->size = 0;
	return tdb;
}

int subscribe(topicDB *tdb, char *tname, char *id, int SF) {
	if (tdb->size == 0) {
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
			s = m + 1;
		} else {
			d = m - 1;
		}
	}
	// Ajung aici daca nu exista topicul respectiv, si trebuie creat.
	tdb->size++;
	for (i = tdb->size - 2; i >= 0; i--) {
		if (strcmp(tdb->tlist[i].name, tname) < 0) {
			tdb->tlist[i + 1].count = 1;
			strcpy(tdb->tlist[i + 1].name, tname);
			tdb->tlist[i + 1].subscribers = (follower *) calloc(MAX_CLIENTS, sizeof(follower));
			strcpy(tdb->tlist[i + 1].subscribers[0].id, id);
			tdb->tlist[i + 1].subscribers[0].SF = SF;
			tdb->tlist[i + 1].count = 1;;
			return 1;	// client adaugat cu succes
		}
		memcpy(&(tdb->tlist[i + 1]), &(tdb->tlist[i]), sizeof(topic));
	}
	tdb->tlist[0].count = 1;
	strcpy(tdb->tlist[0].name, tname);
	tdb->tlist[0].subscribers = (follower *) calloc(MAX_CLIENTS, sizeof(follower));
	strcpy(tdb->tlist[0].subscribers[0].id, id);
	tdb->tlist[0].subscribers[0].SF = SF;
	tdb->tlist[0].count = 1;;
	return 1;// client abonat cu succes
}
int unsubscribe(topicDB *tdb, char *tname, char *id) {
	// if (tdb->size == 1) {
	// 	if (str)
	// 	tdb->size = 0;
	// 	strcpy(tdb->tlist[0].name, "");
	// 	free(tdb->tlist[0].subscribers);
	// 	return;
	// }
	int i, s, d, m;
	s = 0;
	d = tdb->size - 1;
	while (s <= d) {
		m = s + (d - s) / 2;
		if (strcmp(tdb->tlist[m].name, tname) == 0) {
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
			s = m + 1;
		} else {
			d = m - 1;
		}
	}
	return -1; // nu a fost gasit utlizatorul ca abonat al topicului specificat
}
