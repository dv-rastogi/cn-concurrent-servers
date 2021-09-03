#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h> 
#include <arpa/inet.h> 
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <dirent.h>

#define PORT 8080
#define NUM_C 5
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

void get_info(char* buffer, int n) {
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
    for (int i = 0; i < n; ++ i) {
        sprintf(buffer, "%s %s %lu\n", cur_proc[i].pid, cur_proc[i].proc_name, cur_proc[i].total_time);
    }
    closedir(proc);
}

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

    // get top n processes from server
    char response_filename[128] = {0};
    sprintf(response_filename, "client/response_%d.txt", id);
    FILE* response_fp = fopen(response_filename, "w+");
    if (response_fp == NULL) {
        perror("File open failed");
        abort();
    }
    char temp_buffer[512] = {0};
    int read_status;
    while ((read_status = read(client_fd, temp_buffer, sizeof(temp_buffer))) != 0) {
        if (read_status == -1) {
            perror("Read error");
            abort();
        }
        printf("[%d] Client read status: %d\n", client_fd, read_status);
        printf("[%d] Message recieved from server (%d):\n%s", client_fd, id, temp_buffer);
        // check for termination
        const char* msg_end = &temp_buffer[strlen(temp_buffer) - strlen(TERMINATE)];
        if (strcmp(TERMINATE, msg_end) == 0) {
            char last_msg[512] = {0};
            strncpy(last_msg, temp_buffer, strlen(temp_buffer) - strlen(TERMINATE));
            fprintf(response_fp, "%s", last_msg);
            break;
        }
        fprintf(response_fp, "%s", temp_buffer);
        memset(temp_buffer, 0, sizeof(temp_buffer));
    }
    fclose(response_fp);
    printf("\n[%d] Saved in file: %s\n", client_fd, response_filename);

    sleep(5);

    // send its own top process
    char top_proc[512] = {0};
    get_info(top_proc, 1);
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