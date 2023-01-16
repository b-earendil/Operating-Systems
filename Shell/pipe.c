#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>


int main() {
    int pipefd[2]; // array to hold fds

    if(pipe(pipefd)==-1) {
        perror("error: pipe creating in main");
    }

    pid_t pid = fork(); // fork process
 
    if(pid==0) { // if child
        char buf[100];
        dup2(pipefd[0], STDIN_FILENO); // redirect stdin to pipe
        int x = read(STDIN_FILENO, buf, sizeof(buf)); // read message from parent
        buf[x] = 0; // attach null terminating character
        printf("child repeats: %s\n", buf); // print message
    } else { // else, parent
        dup2(pipefd[1], STDOUT_FILENO); // redirect stdout to pipe
        printf("'this message from the parent'\n"); // write message to child
        fprintf(stderr, "parent wrote to pipe\n");
    }
    return 0;
}