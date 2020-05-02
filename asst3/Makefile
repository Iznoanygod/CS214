all:
	gcc client.c -o WTF simpleIO.o
client:
	gcc client.c -o WTF simpleIO.o
server:
	gcc server.c -g -pthread -lm -o WTFserver simpleIO.o
lib:
	gcc simpleIO.c -g -c -o simpleIO.o
clean:
	rm WTF
	rm simpleIO.o
	rm WTFserver

