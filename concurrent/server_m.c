#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <string.h> 
#include <sys/fcntl.h> 
#include <unistd.h>
#include <pthread.h>

#define PORT 8080
#define WAIT_LIMIT 50

void* socket_thread(void *arg) {
    int sock_fd = *((int *) arg);
    char client_msg[1024] = {0};
    if (read(sock_fd, client_msg, sizeof(client_msg)) < 0) {
        perror("Receive error");
        abort();
    }
    if (strlen(client_msg))
        printf("[%d] Received from client: %s\n", sock_fd, client_msg);
    sleep(1);
    char *server_msg = "hello world";
    if (send(sock_fd, server_msg, strlen(server_msg), MSG_NOSIGNAL) < 0) {
        perror("Send error");
        abort();
    }
    printf("[%d] Message sent & closing socket\n", sock_fd);
    close(sock_fd);
    free(arg);
    pthread_exit(NULL);
}

int main() {
    // server socket
    int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_fd == 0) {
        perror("Server socket creation failed");
        exit(1);
    }
    printf("Server socket: %d\n", serv_fd);

    // reuse the socket in case of previous occupance
    int flag = 1;
    setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    
    // bind server socket
    struct sockaddr_in serv_addr;
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = (htonl) INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    if (bind(serv_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Server socket error binding");
        exit(1);
    }

    // listen
    listen(serv_fd, WAIT_LIMIT);
    printf("Listen started\n");

    struct sockaddr_storage serv_storage;
    socklen_t addr_sz;
    pthread_t tid[WAIT_LIMIT];
    int cur_t = 0;
    while (1) {
        addr_sz = sizeof(serv_storage);
        printf("Waiting for client!\n");
        int sock_fd = accept(serv_fd, (struct sockaddr*) &serv_storage, &addr_sz);
        if (sock_fd < 0) {
            perror("Socket accept, new socket creation error");
            exit(1);
        }
        printf("Got client on %d!\n", sock_fd);

        int *arg = malloc(sizeof(*arg));
        *arg = sock_fd;
        if (pthread_create(&tid[cur_t ++], NULL, socket_thread, arg) != 0) {
            perror("Thread creation failed");
            exit(1);
        }

        if (cur_t >= WAIT_LIMIT) {
            for (int i = 0; i < WAIT_LIMIT; ++ i) {
                pthread_join(tid[i], NULL);
            }
            cur_t = 0;
        }
    }

    close(serv_fd);
    return 0;
}