    
    Tema #2 Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

Student: Careja Alexandru-Cristian
Grupa: 324CD



<<+>> Modul de comunicare intre clientii TCP, UDP si server

(+) Tipuri de mesaje peste TCP:
	
	1. String

		char buffer[10]; 
	
	Folosit in locuri specifice:
		=> dupa acceptarea unui client nou pentru a primi id-ul sau
		=> dupa conectarea la server pentru a primi un "ok" sau "existing" drept
		notificare in urma actiunii. La primirea stringului "ok", actiunea s-a
		realizat cu succez. La primirea stringului "existing" subscriberul este
		anuntat ca exista deja un utilizator cu id-ul sau conectat si online. 
		=> dupa un mesaj de (dez)abonare pentru a primi un "ok", "ex", "nex"
		drept notificare in urma actiunii.
			"ok"  - succes
			"ex"  - deja abonat la topicul la care a incercat sa se aboneze
			"nex" - nu este abonat la topocul de la care se incearca dezabonarea
	
	2. Subscribe Message

		typedef struct subscribe_message {
			char subscribe;
			char topic[TOPIC_LEN];
			char SF;
		} subscribe_message;

	Folosit numai pentru trimiterea mesajelor de abonare de la un client TCP
	catre server.
		=> campul subscribe (1 byte) - ia valorile 0/1 in functie de actiune
			0 - unsubscribe
			1 - subscribe
		=> campul topic (fix 50 bytes) - string ce reprezinta topicul
		=> campul SF (1 byte) - ia valorile 0/1 in functie de setarea preferata
		de abonat.
			0 - abonare basic
			1 - abonare cu Store & Forward

	3. TCP Message

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

	Folosit pentru redirectionarea mesajelor UDP catre clientii TCP de catre
	server. Contine un header, definit mai sus pentru asigurarea integritatii
	mesajelor TCP si are 2 campuri:
		=> len - lungimea mesajului, exceptand lungimea header-ului
		=> udp_client - o structura de tip sockaddr_in ce contine informatiile
		clientului udp care a creat postarea (informatiile care ne intereseaza
		sunt mai exact adresa IP si portul)
	Structura tcp_message dispune de urmatoarele campuri:
		=> header - explicat mai sus
		=> topic (fix 50 bytes) - string ce reprezinta topicul
		=> data_type (1 byte) - identificator pentru tipul mesajului (0/1/2/3)
		=> payload (maxim 1500 bytes) - campul in care se retine mesajul, de
		orice tip ar fi el

(+) Pentru UDP exista un singur tip de mesaj, cel prin care clientii udp trimit
datagrame catre server.

		typedef struct udp_message {
			char topic[TOPIC_LEN];
			uint8_t data_type;
			char payload[MAX_PAYLOAD_SIZE];
		} udp_message;

	Campurile structurii au aceeasi semnificatie ca si cele ale structurii 
	tcp_message, cu acelasi nume.



<<+>> Comenzi de la tastatura suportate de server si clienti TCP

(+) Server
	Dupa pornire in terminal, serverul suporta doar comanda 'exit' primita de la
tastatura, insa orice alta comanda tastata nu va afecta rularea programului,
aceasta fiind ignorata

(+) Client TCP
	Dupa pornirea in terminal, un client tcp suport 3 comenzi de la tastatura,
iar introducerea unei alte comenzi decat cele mentionate mai jos va produce un
mesaj de avertizare, dar nu va afecta rularea programului in continuare.
	=> subscribe <topic> <SF>
		Va trimite un mesaj de abonare la un topic, cu SF setat conform 
	inputului, catre server la topicul specificat. Nespecificarea parametrului
	SF, sau specificarea unei valori diferite de 0/1 va rezulta intr-un mesaj de
	avertizare la stdout.

	=> unsubscribe <topic>
		Va trimit un mesaj de dezabonare de la topicul specificat catre server.
	Orice caractere adaugate dupa topic (si separate de spatiu de acesta) vor fi
	ignorate.

	=> exit
		Va duce la inchiderea clientului si implicit a anuntarii serverului ca
	respectivul client TCP s-a deconectat.



<<+>> Modul de functionare al serverului

	Serverul deschide doi socketi, unul UDP pentru comunicarea cu clientii care
trimit datagrame pentru abonati, si un socket TCP pentru ascultat (prin care sa
se conecteze clientii tpc la server). Foloseste o multime de file descriptori in
care retine file descriptorii de la care poate primi mesaje, multime care pentru
inceput va contine fd pentru comunicatia UDP, fd pentru socketul de ascultat
conexiuni TCP si fd pentru stdin (tastatura).
	Intr-un while infinit serverul se foloseste de functia select pentru
multiplexarea intarilor. Cand primeste un mesaj, severul verifica de pe ce fd
a venit acesta, si exista 4 cazuri:

	1) Input de la stdin, atunci cand se scrie in server ceva. Singurul input
	care va modifica starea serverului este comanda 'exit' care va inchide
	serverul si toate conexiunile acestuia.

	2) Un mesaj pe socketul TCP pasiv, atunci cand un client incearca sa se
	conecteze la server. Serverul accepta conexiunea si asteapta un mesaj de
	tip String cu id-ul clientului. Dupa primirea acestuia, verifica daca acest
	id este deja folosit de alt client sau daca este vorba despre o reconectare
	a unui client care s-a delogat anterior. Daca se afla ca este o reconectare,
	atunci trimite clientului, daca exista, mesajele in asteptare pentru el.
	Daca id-ul este deja folosit de alt client, serverul ii va trimite un mesaj
	de avertizare clientului si va inchide conexiunea cu acesta. Daca id-ul este
	valid, clientul va primi un mesaj de tip String cu continutul "ok" si file
	descriptorul sau va fi adaugat in multimea de file descriptori.

	3) Un mesaj pe un socket TCP activ, al unui client TCP, atunci cand un
	client incearca sa se aboneze sau dezaboneze de la un topic, sau atunci cand
	un client TCP s-a deconectat. Daca mesajul primit este de lungime 0, atunci
	inseamna ca respectivul client s-a deconectat si este updatat statusul sau
	si inchis socketul. Daca mesajul nu este de lungime 0, atunci inseamna ca
	un client TCP a trimis o cerere (un)subscribe. Serverul verifica daca se
	poate realiza actiunea, si o duce la capat daca se poate, in final trimitand
	un mesaj de tip String clientului cu rezultatul actiunii("ok", "ex", "nex").

	4) Un mesaj pe socketul UDP, atunci cand un client UDP trimite datagrame
	serverului. Serverul citeste topicul postarii si cere lista de abonati ai
	topicului. Formateaza mesajul pentru a putea fi trimis clientului UDP, adica
	calculeaza lungimea payload-ului + topic + data_type, pune in header
	aceasta lungime si copiaza structura sockaddr_in ce contine date despre
	sursa mesajelor si copiaza datele din mesajul UDP in structura de mesaj TCP.
	Pentru fiecare urmaritor online, trimite mesajul pe loc. Pentru fiecare
	urmaritor care este online si are optiunea Store & Forward activata pentru
	acel topic, retine mesajul asociat lui.



<<+>> Modul de functionare al unui client TCP (subscriber)

	Clientul TCP deschide un scoket TCP pentru comunicat cu serverul si trimite
o cerere de conexiune catre acesta. Apoi trimite un mesaj de tip String cu id-ul
sau si asteapta un raspuns, tot de tip String pentru a afla daca id-ul ales este
valid. Daca acesta nu este valid, atunci clientul va afisa un mesaj informator
la stdout si se va inchide. Daca id-ul este unul valid, atunci clientul isi
creeaza un set de file descriptori ce va contine fd conexiunii TCP si fd pentru
stdin. Intr-un while infinit, folosind functia select se realizeaza
multiplexarea intarilor:

	1) Input de la stdin, atunci cand utilizatorul scrie o comanda. Se verifica
	daca aceasta este una valida ("exit", "subscribe", "unsubscribe") si daca a
	fost apelata cu parametrii corecti. Daca pentru o comanda de subscribe
	lipseste SF sau are alta valoare in afara de 0/1 atunci clientul va afisa un
	mesaj si va renunta la acel input. Daca pentru aceleasi comenzi lipseste
	parametrul topic, clientul va afisa un mesaj si va renunta la acel input.
	Daca se primeste comanda "exit", clientul va inchide conexiunea cu severul.
	Daca se primeste o comanda valida, atunci clientul va trimite un mesaj de
	tip Subscribe message serverului si va astepta un mesaj de tip String drept
	feedback la cererea facuta, anuntand clientul cu un mesaj la stdout daca
	ceva nu a decurs bine.

	2) Un mesaj pe socketul TCP, atunci cand serverul ii trimite clientului
	mesaje de la topicurile la care s-a abonat. Clientul face recieve mai intai
	pe headerul mesajului, cu sizeof(tcp_header), pentru a afla lungimea
	efectiva a mesajului transmis si pentru evitarea citirii unui mesaj diferit
	concatenat la acesta. Clientul face recv pe lungimea specificata in header
	si primeste informatiile pe care le parseaza in functia printMessage, care
	formateaza mesajul in functie de tip, dupa cum este explicat in enuntul
	temei.
