#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char *CMD = "ps -eo pid,cmd,%mem,%cpu --sort=-%cpu | head";    

int main(int argc, char **argv) {
    /**
     * argv[1] --> number of processes
     * argv[2] --> thread / process id (Output file identifier)
     */

    // set output file
    char *out_file;
    int num_proc;
    if (argc < 2) {
        out_file = "./server/proc0.txt";
        num_proc = 10;
    } else if (argc == 2) {
        out_file = "./server/proc0.txt";
        num_proc = atoi(argv[1]);
    } else {
        num_proc = atoi(argv[1]);
        out_file = (char*) calloc(1024, sizeof(char));
        sprintf(out_file, "./server/proc%s.txt", argv[2]);
    }
    printf("Output file: %s\n", out_file);
    
    // update command to get top n processes
    char *cmd = (char*) calloc(2048, sizeof(char));
    sprintf(cmd, "%s -n %d", CMD, num_proc + 1);

    // execute command w/ pipe
    char buffer[1024];
    FILE *fp = popen(cmd, "r");
    if (fp == NULL) {
        perror("Pipe creation error");
        exit(1);
    }

    // write pipe output to output file
    FILE *fp_out = fopen(out_file, "w+");
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        fputs(buffer, fp_out);
    }

    fclose(fp_out);
    pclose(fp);

    return 0;
}