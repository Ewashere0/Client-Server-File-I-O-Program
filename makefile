
#Assignment4 makefile
#

all: ppServer pqClient

ppServer: ppServer.o
	gcc -o ppServer ppServer.o

ppServer.o: ppServer.c ppServer.h
	gcc -c ppServer.c ppServer.h
	
pqClient: pqClient.o
	gcc -o pqClient pqClient.o -pthread

pqClient.o: pqClient.c ppServer.h
	gcc -c pqClient.c ppServer.h
	
clean: 
	rm -f *.o
