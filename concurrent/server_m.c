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
#include <dirent.h>

#define PORT 8080
#define WAIT_LIMIT 50
char *TERMINATE = "$$$";

struct process {
    char pid[256];
    char proc_name[128];
    u_int64_t total_time;
};

static int comp_process(const void* a, const void* b) {
    const struct process* pa = a;
    const struct process* pb = b;
    if (pa->total_time > pb->total_time) return -1;
    else if (pa->total_time == pb->total_time) return 0;
    else return 1;
}

static int check_num(char* s) {
    for (size_t i = 0; i < strlen(s); ++ i)
        if (!(s[i] >= '0' && s[i] <= '9')) return 0;
    return 1;
}

void read_proc(char* pid, u_int64_t *utime, u_int64_t *stime, char* proc_name) {
    char proc_path[1024] = {0};
    sprintf(proc_path, "/proc/%s/stat", pid);
    FILE* fp = fopen(proc_path, "r");
    if (fp == NULL) {
        printf("Proc pid: %s", pid);
        perror("Process read failed");
        exit(1);
    }
    // read the 14th and 15th value for utime and stime
    for (int ignore = 0; ignore < 13; ++ ignore) {
        if (ignore == 1) fscanf(fp, "%s", proc_name); // 2nd entry is name
        else fscanf(fp, "%*s");
    }
    fscanf(fp, "%lu", utime);
    fscanf(fp, "%lu", stime);
    fclose(fp);
}

void get_info(char* client_file, int n) {
    DIR *proc = opendir("/proc");
    if (proc == NULL) {
        perror("Opendir failed");
        abort();
    }
    struct process cur_proc[8192];
    memset(cur_proc, 0, sizeof(cur_proc));
    int proc_counter = 0;
    struct dirent *f = readdir(proc);
    do {
        if (!check_num(f->d_name)) goto next;
        char *pid = f->d_name;

        u_int64_t utime, stime;
        char proc_name[128] = {0};
        read_proc(pid, &utime, &stime, proc_name);
        // printf("name: %s, utime: %lu, stime: %lu\n", proc_name, utime, stime); DEBUG

        strcpy(cur_proc[proc_counter].pid, pid);
        strcpy(cur_proc[proc_counter].proc_name, proc_name);
        cur_proc[proc_counter].total_time = utime + stime;
        proc_counter ++;
        next:
            f = readdir(proc);
    } while (f != NULL);
    // sorting processes
    qsort(cur_proc, proc_counter, sizeof(cur_proc[0]), comp_process);
    FILE* client_fp = fopen(client_file, "w+");
    for (int i = 0; i < n; ++ i) {
        fprintf(client_fp, "%s %s %lu\n", cur_proc[i].pid, cur_proc[i].proc_name, cur_proc[i].total_time);
    }
    fclose(client_fp);
    closedir(proc);
}

void* socket_thread(void *arg) {
    int sock_fd = *((int *) arg);
    // read from client
    char client_msg[64] = {0};
    if (read(sock_fd, client_msg, sizeof(client_msg)) < 0) {
        perror("Receive error");
        abort();
    }    
    printf("[%d] Connected to client: %s\n", sock_fd, client_msg);

    // get client id & n
    int client_id, n;
    sscanf(client_msg, "%d %d", &client_id, &n);
    
    // write to local file
    char client_filename[128] = {0};
    sprintf(client_filename, "server/request_%d.txt", client_id);
    get_info(client_filename, n);
    printf("[%d] Saved in file: %s\n", sock_fd, client_filename);

    // read file to send to client
    char temp_buffer[512] = {0};
    FILE* fp = fopen(client_filename, "r");
    if (fp == NULL) {
        perror("File read failed");
        abort();
    }
    int line_no = 1;
    while (fgets(temp_buffer, sizeof(temp_buffer), fp) != NULL) {
        printf("[%d] Sending line %d: %s", sock_fd, line_no ++, temp_buffer);
        if (send(sock_fd, temp_buffer, strlen(temp_buffer), MSG_NOSIGNAL) < 0) {
            perror("Send error");
            abort();
        }
        memset(temp_buffer, 0, sizeof(temp_buffer));
    }
    fclose(fp);
    if (send(sock_fd, TERMINATE, strlen(TERMINATE), MSG_NOSIGNAL) < 0) {
        perror("Termination msg send error");
        abort();
    }
    
    // wait for response from client for top process
    char top_process[512] = {0};
    if (read(sock_fd, top_process, sizeof(top_process)) < 0) {
        perror("Receive error");
        abort();
    }
    printf("[%d] Top process of client received as {pid name time_spent}: %s", sock_fd, top_process);

    printf("[%d] Closing socket\n", sock_fd);
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
        printf("Waiting for client...\n");
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