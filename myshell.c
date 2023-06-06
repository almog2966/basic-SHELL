#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h>

void enable_SIGINT();
void disable_SIGINT();

int run_process(int count, char**commands);
int input_redirecting(int count, char**commands);
int run_process_with_pipe(int idx, char** commands);
int run_process_in_background(int count,char **commands);



int contain_pipe(int count, char** arglist);


int finalize(void)
{
    return 0;
}

int prepare(void)
{
    disable_SIGINT();
    return 0;
}

void enable_SIGINT()
{
    /*the process will terminate upon SIGINT*/
    if (signal(SIGINT, SIG_DFL) == SIG_ERR) {
        /*enabling the signal SIGINT*/
        fprintf(stderr,"An Error Has Occourd!");
        exit(1);
    }
}

void disable_SIGINT()
{
    /*after prepare() finishes, the parent(shell) will not terminate upon SIGINT*/
    if (signal(SIGINT, SIG_IGN) == SIG_ERR) {
        /*ignoring the signal SIGINT*/
        fprintf(stderr,"An Error Has Occourd!");
        exit(1);
    }
}



int process_arglist(int count, char** arglist)
{
    int idx;
    if(count == 0)
        return 0;
    if(strcmp(arglist[count - 1], "&") == 0)
    {
        arglist[count-1] = NULL; /*ignoring the '&' symbol*/
        return run_process_in_background(count,arglist);
    }
    else if ((idx = contain_pipe(count,arglist))!=-1)
    {
        arglist[idx] = NULL; /*ignoring the '|' symbol*/
        return run_process_with_pipe(idx,arglist);
    }
    else if (strcmp(arglist[count - 1], "<") == 0)
    {
        arglist[count-2] = NULL;
        return input_redirecting(count,arglist);
    }
    else
    {
        return run_process(count,arglist);
    }
    return 1;
}
int run_process(int count, char**commands)
{
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr,"An Error Has Occourd!");
        exit(1);
    }
    /*executing the process*/
    if(pid == 0)
    {
        enable_SIGINT();
        if(execvp(commands[0],commands) == -1)
        {
            fprintf(stderr,"An Error Has Occourd!");
            return 0;
        }
    }

    /*parent*/
    if (waitpid(pid, NULL, 0) == -1 && errno != ECHILD && errno != EINTR) 
    {
        /*waiting for process to finish, ignoring ECHIL, EINTR errors*/
        fprintf(stderr,"An Error Has Occourd!");
        return 0;
    }
    return 1;

}
int input_redirecting(int count, char**commands)
{
    pid_t pid = fork();
    if (pid == -1) {
        fprintf(stderr,"An Error Has Occourd!");
        exit(1);
    }
    /*executing the process*/
    if (pid == 0)
    {
        enable_SIGINT();
        /*openning the file as read only file, creating if it doesnt exists*/
        int file = open(commands[count-1],O_RDONLY | O_CREAT, 0644);
        if(file == -1)
        {
            fprintf(stderr,"An Error Has Occourd!");
            exit(1);
        }
        /*refering STDIN to the file*/
        if(dup2(file,STDIN_FILENO) == -1)
        {
            fprintf(stderr,"An Error Has Occourd!");
            exit(1);
        }
        close(file);
        /*executing the process*/
        if (execvp(commands[0], commands) == -1) 
        {
            fprintf(stderr,"An Error Has Occourd!");
            return 0;
        }
    }
    /*parent*/
    if (waitpid(pid, NULL, 0) == -1 && errno != ECHILD && errno != EINTR) {
        /*waiting for first process to finish, ignoring ECHIL, EINTR errors*/
        fprintf(stderr,"An Error Has Occourd!");
        return 0;
    }

    return 1;

}
int contain_pipe(int count, char** arglist)
{
    /* return the index of the '|' symbole if exists, return -1 otherwise*/
    int i;
    for(i=0; i < count; i++)
        if (strcmp(arglist[i], "|") == 0)
            return i;
    return -1;
}
int run_process_with_pipe(int idx, char** commands)
{
    int pfds[2];
    if (pipe(pfds) == -1) {
        fprintf(stderr,"An Error Has Occurod!");
        exit(1);
    }
    /*first process creation*/
    pid_t first_process = fork();
    if (first_process == -1) {
        fprintf(stderr,"An Error Has Occurod!");
        exit(1);
    }
    /*handling the first process*/
    else if(first_process == 0)
    {
        enable_SIGINT();
        close(pfds[0]); /* close the reading side of the pipe*/
        if(dup2(pfds[1],STDOUT_FILENO) == - 1)
        {
            /*refering the pipe write side to STDOUT of first process*/
            fprintf(stderr,"An Error Has Occurod!");
            exit(1);
        }
        close(pfds[1]); /*after */
        if(execvp(commands[0], commands) == -1)
        {
            fprintf(stderr,"An Error Has Occurod!");
            return 0;
        }
    }


    /*second process creation*/
    pid_t second_process = fork();
    if(second_process == -1)
    {
        fprintf(stderr,"ERROR: Fork has failed.");
        exit(1);
    }
    /*handling the second process*/
    if (second_process == 0)
    {
        enable_SIGINT();
        close(pfds[1]);/*closing the write side*/
        if(dup2(pfds[0],STDIN_FILENO) == - 1)
        {
            /*refering the pipe read side to STDIN of the new process */
            fprintf(stderr,"An Error Has Occurod!");
            exit(1);
        }
        close(pfds[0]);
        if(execvp(commands[idx+1], commands + (idx+1)) == -1)
        {
            fprintf(stderr,"An Error Has Occurod!");
            return 0;
        }
    }
    close(pfds[0]);
    close(pfds[1]);


    /*parent - waiting for first process*/
    if(waitpid(first_process, NULL, 0) == -1 && errno != ECHILD && errno != EINTR)
    {
        /*waiting for first process to finish, ignoring ECHIL, EINTR errors*/
        fprintf(stderr,"An Error Has Occurod!");
        return 0;
    }

    /*parent - waiting for second process*/
    if(waitpid(second_process, NULL, 0) == -1 && errno != ECHILD && errno != EINTR)
    {
        /*waiting for second process to finish, ignoring ECHIL, EINTR errors*/
        fprintf(stderr,"An Error Has Occurod!");
        return 0;
    }
    return 1;

}
int run_process_in_background(int count,char **commands)
{
    pid_t pid = fork();
    if(pid == -1)
    {
        fprintf(stderr,"An Error Has Occourd!");
        exit(1);
    }
    if(pid == 0) /*child process*/
    {
        enable_SIGINT();
        if(execvp(commands[0],commands) == -1)
        {
            fprintf(stderr,"An Error Has Occourd!");
            return 0;
        }
    }
    return 1;
}