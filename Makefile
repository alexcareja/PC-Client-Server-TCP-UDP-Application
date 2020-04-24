CC	= gcc
CFLAGS	= -Wall
SV	= server.o
SB	= subscriber.o
TARGETSV= server
TARGETSB= subscriber

all: $(TARGETSV) $(TARGETSB)

$(TARGETSV): $(SV)
	$(CC) $(CFLAGS) -o $(TARGETSV) $(SV)

$(TARGETSB): $(SB)
	$(CC) $(CFLAGS) -o $(TARGETSB) $(SB)

clean:
	rm -f $(SV) $(TARGETSV)
	rm -f $(SB) $(TARGETSB)
