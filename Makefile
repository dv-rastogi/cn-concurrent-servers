c:
	gcc -Wall -Wextra client.c -o _client
	./_client

s:
	gcc -Wall -Wextra server.c -o _server
	./_server

u:
	gcc -Wall -Wextra util.c -o _util
	./_util

clean:
	rm -f _client _server _util