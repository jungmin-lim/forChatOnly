.PHONY: clean
all: server client

RM = rm -f

server: server.o
	gcc -o server server.c -lpthread -D_REENTRANT

client: client.o screen.o bufstring.o
	gcc -o client client.o screen.o bufstring.o -lcurses -lpthread -D_REENTRANT

server.o: server.c
	gcc -c server.c

client.o: client.c
	gcc -c client.c

screen.o: screen.c screen.h
	gcc -c screen.c

bufstring.o: bufstring.c bufstring.h
	gcc -c bufstring.c

clean:
	$(RM) *.o
