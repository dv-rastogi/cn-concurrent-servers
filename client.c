#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <string.h>

#define PORT 8000

int main() {

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Client socket creation failed");
        exit(1);
    }
    printf("Client socket: %d\n", client_fd);

    struct sockaddr_in serv_addr;
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (connect(client_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Client connection error");
        exit(1);
    }

    for (int i = 0; i < 10; ++ i) {
        char message[1024] = {0};
        sprintf(message, "%d", i);
        send(client_fd, message, strlen(message), 0);
        printf("Message sent from client!\n");

        char buffer[1024] = {0};
        read(client_fd, buffer, sizeof(buffer));
        printf("Received from server: %s\n", buffer);
    }

    close(client_fd);
    return 0;
}