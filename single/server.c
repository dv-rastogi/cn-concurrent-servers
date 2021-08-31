#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <string.h> 
#include <sys/fcntl.h> 
#include <unistd.h>

#define PORT 8000
#define WAIT_LIMIT 5

int main() {

    // server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Server socket creation failed");
        exit(1);
    }
    printf("Server socket: %d\n", server_fd);
    
    // bind server socket
    struct sockaddr_in serv_addr;
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = (htonl) INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Server socket error binding");
        exit(1);
    }

    // listen
    listen(server_fd, WAIT_LIMIT);
    printf("Listen started\n");

    struct sockaddr_in cli_addr;
    int cli_len = sizeof(cli_addr);
    int sock_fd = accept(server_fd, (struct sockaddr *) &cli_addr, (socklen_t*) &cli_len);
    if (sock_fd < 0) {
        perror("Socket accept, new socket creation error");
    }

    while (1) {
        // read
        char buffer[1024] = {0};
        int read_status = read(sock_fd, buffer, sizeof(buffer));
        if (read_status < 0) {
            perror("Read terminate");
            break;
        }
        printf("Read %d\n", read_status);
        printf("Client data: %s\n", buffer);

        // respond
        char *affirmation = "Data received";
        int send_status = send(sock_fd, affirmation, strlen(affirmation), MSG_NOSIGNAL);
        if (send_status < 0) {
            perror("Send fail");
            break;
        }
        printf("Server responded (status: %d) \n", send_status);
    }

    // close
    close(server_fd);
    close(sock_fd);
    
    return 0;
}