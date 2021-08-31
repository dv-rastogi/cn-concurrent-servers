c:
	gcc -Wall -Wextra single/client.c -o _client
	./_client

s:
	gcc -Wall -Wextra single/server.c -o _server
	./_server

u:
	gcc -Wall -Wextra util.c -o _util
	./_util

cm:
	gcc -pthread -Wall -Wextra concurrent/client_m.c -o _client_m
	./_client_m

sm: 
	gcc -pthread -Wall -Wextra concurrent/server_m.c -o _server_m
	./_server_m

clean:
	rm -f _client _server _util _client_m _server_m