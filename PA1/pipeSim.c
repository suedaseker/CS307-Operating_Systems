#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    
    printf("I'm SHELL process, with PID: %d - Main command is: man ls | grep \"-S\" > output.txt\n", (int) getpid());

    int pfd[2]; //(0 read, 1 write)
    pipe(pfd);

    if (pipe(pfd) == -1) {
        fprintf(stderr, "Pipe Failed");
        exit(1);
    }

    int rc = fork();
    if (rc < 0) {
        // fork failed
        fprintf(stderr, "fork failed\n");
        exit(1);
    } else if (rc == 0) {
        printf("I'm MAN process, with PID: %d - My command is: man ls\n", (int) getpid());
        close(pfd[0]);
        dup2(pfd[1], STDOUT_FILENO); //instead od write to console, write to pfd[1]
        
        //exec man
        char *myargs[3];
        myargs[0] = strdup("man");
        myargs[1] = strdup("ls");
        myargs[2] = NULL;
        execvp(myargs[0], myargs);
    }else {
        int rc2 = fork();
        if (rc2 < 0) {
            // fork failed
            fprintf(stderr, "fork failed\n");
            exit(1);
        } else if (rc2 == 0) {
            //child of child
            int status;
            waitpid(rc, &status, 0);
            
            printf("I'm GREP process, with PID: %d - My command is: grep \"-S\" > output.txt\n", (int) getpid());
            close(pfd[1]);
            dup2(pfd[0], STDIN_FILENO); //instead od read to console, read from pfd[1]

            close(STDOUT_FILENO);
            open("./output.txt", O_RDWR|O_CREAT|O_TRUNC, 0777);

            //exec grep
            char *myargs[3];
            myargs[0] = strdup("grep");
            myargs[1] = strdup("\\-S");
            myargs[2] = NULL;
            execvp(myargs[0], myargs);

        }else{
        }
        // parent
        wait(NULL);
        printf("I'm SHELL process, with PID: %d - execution is completed, you can find the results in output.txt\n", (int) getpid());
    }
    return 0;     
}