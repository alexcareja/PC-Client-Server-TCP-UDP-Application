#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <netinet/in.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define TOPIC_LEN 50
#define MAX_PAYLOAD_SIZE 1500
#define MAX_STRING_LEN 1500
#define TINT 0
#define TSHORT_REAL 1
#define TFLOAT 2
#define TSTRING 3
#define MAX_CLIENTS 250
#define MAX_TOPICS 250
#define BUFLEN 10
#define ID_LEN 10
#define FLOAT_LEN 6
#define SHORT_REAL_LEN 2
#define INT_LEN 5
#define MAX_SAVED 100

typedef struct udp_message {
	char topic[TOPIC_LEN];
	uint8_t data_type;
	char payload[MAX_PAYLOAD_SIZE];
} udp_message;

typedef struct tcp_header {
	int len;
	struct sockaddr_in udp_client;
} tcp_header;

typedef struct tcp_message {
	tcp_header header;
	char topic[TOPIC_LEN];
	char data_type;
	char payload[MAX_PAYLOAD_SIZE];
} tcp_message;

typedef struct subscribe_message {
	char subscribe;
	char topic[TOPIC_LEN];
	char SF;
} subscribe_message;

typedef struct tcpClient {
	int sockfd;
	char id[ID_LEN];
	int status;
	tcp_message *saved;
	int no_saved;
} tcpClient;

typedef struct cliList {
	tcpClient *cl;
	int size;
} cliList;

typedef struct follower {
	char id[ID_LEN];
	int SF;
} follower;

typedef struct topic {
	char name[TOPIC_LEN];
	follower *subscribers;
	int count;
} topic;

typedef struct topicDB {
	topic *tlist;
	int size;
} topicDB;

cliList *createCliList();
int connectClient(cliList **, int, char *);
char *getClientId(cliList *, int);
int getClientFD(cliList *, char *);
int isClientOnline(cliList *, char *);
void storeMessage(cliList *, char *, tcp_message *);
tcp_message *getSaved(cliList *, char *);
int getSavedCount(cliList *, char *);
void logoutClient(cliList *, int);
void destroyCliList(cliList *);

topicDB *initTDB();
int subscribe(topicDB *, char *, char *, int);
int unsubscribe(topicDB *, char *, char *);
follower *getSubscribers(topicDB *, char *);
int getSubscribersCount(topicDB *, char *);
void destroyTDB(topicDB *);

void printMessage(tcp_message);
#endif
