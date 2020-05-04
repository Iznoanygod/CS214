all:
	gcc simpleIO.c -lssl -lcrypto -g -c -o simpleIO.o
	gcc client.c -lssl -lcrypto -g -o WTF simpleIO.o
	gcc server.c -g -lssl -lcrypto -pthread -lm -o WTFserver simpleIO.o
client:
	gcc simpleIO.c -lssl -lcrypto -g -c -o simpleIO.o
	gcc client.c -lssl -lcrypto -g -o WTF simpleIO.o
server:
	gcc simpleIO.c -lssl -lcrypto -g -c -o simpleIO.o
	gcc server.c -g -lssl -lcrypto -pthread -lm -o WTFserver simpleIO.o
lib:
	gcc simpleIO.c -lssl -lcrypto -g -c -o simpleIO.o
clean:
	rm WTF
	rm simpleIO.o
	rm WTFserver

