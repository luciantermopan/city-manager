#include <signal.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>

int primit_semnal_stop = 0;

void trateaza_SIGUSR1(int p)
{
    printf("Message printed by SIGUSR1!\n");
    return;
}

void trateaza_SIGINT(int p)
{
    printf("Message printed by SIGINT!\n");

    pid_t pid_copil = fork();
    if(pid_copil == -1)
    {
        printf("nu s-a putut crea un copil");
        primit_semnal_stop = 1;
    }
    else if(pid_copil == 0)
    {
        execlp("rm", "rm", "-rf", ".monitor_pid", NULL);
        exit(1);
    }
    else
    {
        wait(NULL); 
        primit_semnal_stop = 1;
    }
    return;
}

int main()
{
    /*int open(const char *path, int flags, ...mode_t mode)*/
    mode_t file_mode = 0750; // same as containing folder

    int fd_w = open(".monitor_pid", O_CREAT | O_TRUNC | O_WRONLY, file_mode);
    if(fd_w == -1)
    {
        printf("Error in creating the .monitor_pid file!\n");
        exit(-1);
    }

    // finding the process' id
    pid_t process_id = getpid();
        
    if( write(fd_w, &process_id, sizeof(pid_t)) != sizeof(pid_t) )
    {
        printf("Unable to write the necessary amount of bytes to .monitor_pid file!\n");
        close(fd_w);
        exit(-1);
    }

    struct sigaction st;
    memset(&st, 0, sizeof(st));

    // setting for SIGINT
    st.sa_handler = trateaza_SIGINT;
    sigemptyset(&st.sa_mask);
    if(sigaction(SIGINT, &st, NULL) == -1) 
    {
        printf("eroare setare signal handler SIGINT!\n");
        exit(-1);
    }

    // setting for SIGUSR1
    st.sa_handler = trateaza_SIGUSR1;
    sigemptyset(&st.sa_mask);    
    if(sigaction(SIGUSR1, &st, NULL) == -1) 
    {
        printf("eroare setare signal handler SIGUSR1!\n");
        exit(-1);
    }

    // printf("monitor_reports PID: %d\n", process_id);
    // printf("waiting signal... (SIGINT(2) to stop, SIGUSR1(10) for new reports)\n");
    while(primit_semnal_stop != 1)
    {
        usleep(1); // executare infinita pana cand variabila se pune pe 1
    }

    close(fd_w);
    return 0;
}