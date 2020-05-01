all:
	gcc client.c -o WTF
client:
	gcc client.c -o WTF
server:
	gcc server.c -g -lz -ltar -pthread -lm -o WTFserver simpleIO.o
lib:
	gcc simpleIO.c -g -c -o simpleIO.o
clean:
	rm WTF
	rm simpleIO.o
	rm WTFserver

