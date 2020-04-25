build:
	gcc -Wall server.c -c
	gcc -Wall subscriber.c -c
	gcc -Wall helpers.c -c
	gcc -Wall server.o helpers.o -o server
	gcc -Wall subscriber.o helpers.o -o subscriber

clean:
	rm -f *.o
	rm -f server
	rm -f subscriber