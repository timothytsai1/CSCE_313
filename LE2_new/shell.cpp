/****************
LE2: Introduction to Unnamed Pipes
****************/
#include <unistd.h> // pipe, fork, dup2, execvp, close
#include <iostream>
#include <sys/wait.h>
using namespace std;
using std::cout;

int main()
{
    // lists all the files in the root directory in the long format
    char *cmd1[] = {(char *)"ls", (char *)"-al", (char *)"/", nullptr};
    // translates all input from lowercase to uppercase
    char *cmd2[] = {(char *)"tr", (char *)"a-z", (char *)"A-Z", nullptr};

    // TODO: add functionality
    // Create pipe
    int fd[2];
    pid_t pid;

    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stdin = dup(STDIN_FILENO);

    if (pipe(fd) == -1)
    {
        std::cout << "Error with pipe creation";
    }
    // Create child to run first command
    
    pid = fork();
    if (pid == 0)
    {
            //[0,fd[1],2]
        dup2(fd[1], STDOUT_FILENO); // In child, redirect output to write end of pipe
        close(fd[0]);               // Close the read end of the pipe on the child side.
        close(fd[1]);

        execvp(cmd1[0], cmd1); // In child, execute the command
    }

    pid = fork();
    if (pid == 0)
    {
        // Create another child to run second command
        //[fd[0],1,2]
        dup2(fd[0], STDIN_FILENO); // In child, redirect input to the read end of the pipe
        close(fd[1]);              // Close the write end of the pipe on the child side.
        close(fd[0]);
        execvp(cmd2[0], cmd2);     // Execute the second command.
    }

    close(fd[0]);
    close(fd[1]);
    // Reset the input and output file descriptors of the parent.

    wait(NULL);
    wait(NULL);

    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stdin, STDIN_FILENO);

    close(saved_stdout);
    close(saved_stdin);
}
