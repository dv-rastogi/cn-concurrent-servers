#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char *CMD = "ps -eo pid,cmd,%mem,%cpu --sort=-%cpu | head -n 11";    

int main(int argc, char **argv) {

    // set output file
    char *out_file;
    if (argc < 2) {
        out_file = "./server/proc0.txt";
    } else {
        out_file = (char*) calloc(1024, sizeof(char));
        sprintf(out_file, "./server/proc%s.txt", argv[1]);
    }
    printf("Output file: %s\n", out_file);

    // execute command w/ pipe
    char buffer[1024];
    FILE *fp = popen(CMD, "r");
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