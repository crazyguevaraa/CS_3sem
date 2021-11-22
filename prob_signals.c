#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/prctl.h>




int bit = 0;

void Handler (int signo);


int main (int argc, char* argv []){

    int fd = open (argv [1], O_RDONLY);
    if (fd == -1){
        printf ("cannot open file\n");
        exit (1);
    }

    struct sigaction act {};
    act.sa_handler = Handler;

    sigaction (SIGUSR1, &act, NULL);
    sigaction (SIGUSR2, &act, NULL);
    sigaction (SIGTERM, &act, NULL);
    sigaction (SIGCHLD, &act, NULL);

    sigset_t mask;

    sigemptyset (&mask);
    sigaddset (&mask, SIGUSR1);
    sigaddset (&mask, SIGUSR2);
    sigprocmask (SIG_BLOCK, &mask, NULL);

    int parent_id = getpid ();
    
    int child_stat = fork ();
//in child proc
    if (child_stat == 0){

    }



}

void Handler (int signo){
    if (signo == SIGUSR1){
        bit = 0;
     }

    else if (signo == SIGUSR2){
        bit = 1;
     }

    else if (signo == SIGCHLD){ 
        
        int status = 0;
        wait (&status);
        
        if (status != 0){
            printf ("child is dead\n");
            exit (1);
        }
        exit (0);

     }

    else if (signo == SIGTERM){
        printf ("terminate\n");
        exit (1);
     }

}