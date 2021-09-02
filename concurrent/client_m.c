#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define PORT 8080
#define NUM_C 7

double total_time = 0;
pthread_mutex_t lock;

void *client_thread(void *arg) {
    clock_t t = clock();
    int id = *((int *) arg);
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Client socket creation failed");
        abort();
    }
    printf("[%d] Client socket for %d\n", client_fd, id);

    struct sockaddr_in serv_addr;
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (connect(client_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Client connection error");
        abort();
    }

    // send n: Top n processes to retrieve
    int n = rand() % 20 + 1;
    char message[64] = {0};
    sprintf(message, "%d %d", id, n);
    if (send(client_fd, message, strlen(message), 0) < 0) {
        perror("Send error");
        abort();
    }
    printf("[%d] Message sent from client (%d) for %d processes!\n", client_fd, id, n);

    char response_filename[128] = {0};
    sprintf(response_filename, "client/response_%d.txt", id);
    FILE* response_fp = fopen(response_filename, "w+");
    if (response_fp == NULL) {
        perror("File open failed");
        abort();
    }
    char buffer[8192] = {0};
    if (read(client_fd, buffer, sizeof(buffer)) > 0) {
        printf("[%d] Message recieved from server (%d):\n%s", client_fd, id, buffer);
        fprintf(response_fp, "%s", buffer);
    } else {
        perror("Read error");
        abort();
    }
    fclose(response_fp);
    printf("[%d] Saved in file: %s\n", client_fd, response_filename);

    FILE* top_proc_fp = fopen(response_filename, "r");
    if (top_proc_fp == NULL) {
        perror("File read failed");
        abort();
    }
    char top_proc[512] = {0};
    fgets(top_proc, sizeof(top_proc), top_proc_fp);
    fclose(top_proc_fp);
    if (send(client_fd, top_proc, strlen(top_proc), 0) < 0) {
        perror("Send error");
        abort();
    }
    printf("[%d] Top process sent from client (%d): %s", client_fd, id, top_proc);

    close(client_fd);
    free(arg);

    t = clock() - t;
    double time_taken = (double) t / CLOCKS_PER_SEC;
    pthread_mutex_lock(&lock);
    total_time += time_taken;
    pthread_mutex_unlock(&lock);

    pthread_exit(NULL);
}

int main() {
    srand((unsigned int) time(NULL));
    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("Mutex init failed");
        exit(1);
    }
    pthread_t tid[NUM_C];
    for (int cur_t = 0; cur_t < NUM_C; ++ cur_t) {
        int *arg = malloc(sizeof(*arg));
        *arg = cur_t;
        if (pthread_create(&tid[cur_t], NULL, client_thread, arg) != 0) {
            perror("Client thread creation failed");
            exit(1);
        }
    }
    for (int i = 0; i < NUM_C; ++ i) {
        pthread_join(tid[i], NULL);
    }
    // store average response time
    double avg_time = total_time / NUM_C;
    printf("==> Avg time taken for %d clients: %lfs\n", NUM_C, avg_time);
    char file_name[128] = {0};
    sprintf(file_name, "response_time/%d.txt", NUM_C);
    FILE *fp = fopen(file_name, "w+");
    if (fp == NULL) {
        perror("File open failed");
        exit(1);
    }
    fprintf(fp, "%lf", avg_time);
    fclose(fp);
    printf("==> Stored in %s\n", file_name);
    return 0;
}