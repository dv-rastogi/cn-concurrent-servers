## Concurrent server in C:
Concurrent server in C to query CPU utilisation of server and client via TCP socket programming.

1. Run `make sm` to start the server (Source code: __concurrent/server_m.c__)
2. Run `make cm` to start the client (Source code: __concurrent/client_m.c__)

* You can configure NUM_C in __concurrent/client_m.c__ to the number of clients.
* Server files are stored in directory __server/__.
* Client files are stored in directory __client/__.
* Finally run: `python3 analysis.py` to visualise _average response time vs number of clients_.
