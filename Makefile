c:
	gcc -Wall client.c -o client
	./client

s:
	gcc -Wall server.c -o server
	./server

clean:
	rm -f client server