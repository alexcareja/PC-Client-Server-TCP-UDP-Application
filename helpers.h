#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

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
#define INT 0
#define SHORT_REAL 1
#define FLOAT 2
#define STRING 3
#define MAX_CLIENTS 250
#define MAX_TOPICS 250
#define BUFLEN 10
#define ID_LEN 10

typedef struct udp_message {
	char topic[TOPIC_LEN];
	char data_type;
	char payload[MAX_PAYLOAD_SIZE];
} udp_message;

typedef struct subscribe_message {
	char subscribe;
	char topic[TOPIC_LEN];
	char SF;
} subscribe_message;

typedef struct subsList {
	char topic[TOPIC_LEN];
	struct subsList *next;
} subsList;

typedef struct tcpClient {
	int sockfd;
	char id[ID_LEN];
	int status;
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

subsList *createNode(char *);
subsList *removeTopic(subsList *, char *);
int findTopic(subsList *, char *);

cliList *createCliList();
int connectClient(cliList **, int, char *);
char *getClientId(cliList *, int);
int getClientFD(cliList *, char *);
int isClientOnline(cliList *, char *);
void logoutClient(cliList *, int);
void destroyCliList(cliList *);

topicDB *initTDB();
int subscribe(topicDB *, char *, char *, int);
int unsubscribe(topicDB *, char *, char *);
#endif
