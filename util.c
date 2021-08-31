#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

int check_num(char* s) {
    for (size_t i = 0; i < strlen(s); ++ i)
        if (!(s[i] >= '0' && s[i] <= '9')) return 0;
    return 1;
}

void read_proc(char* pid, u_int64_t *utime, u_int64_t *stime, char* proc_name) {
    char proc_path[1024] = {0};
    sprintf(proc_path, "/proc/%s/stat", pid);
    FILE* fp = fopen(proc_path, "r");
    if (fp == NULL) {
        printf("proc pid: %s", pid);
        perror("process read failed");
        exit(1);
    }
    // read the 14th and 15th value for utime and stime
    for (int ignore = 0; ignore < 13; ++ ignore) {
        if (ignore == 1) fscanf(fp, "%s", proc_name); // 2nd entry is name
        else fscanf(fp, "%*s");
    }
    fscanf(fp, "%lu", utime);
    fscanf(fp, "%lu", stime);
}

int main() {
    
    DIR *proc = opendir("/proc");
    if (proc == NULL) {
        perror("opendir failed");
        exit(1);
    }

    struct dirent *f = readdir(proc);
    do {
        if (!check_num(f->d_name)) goto next;
        char *pid = f->d_name;
        printf("reading: %s\n", pid);

        u_int64_t utime, stime;
        char proc_name[1024] = {0};
        read_proc(pid, &utime, &stime, proc_name);

        printf("name: %s, utime: %lu, stime: %lu\n", proc_name, utime, stime);

        next:
        f = readdir(proc);
    } while (f != NULL);


    return 0;
}