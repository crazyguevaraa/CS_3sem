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
int ConvertBin (char character, int number);

int main (int argc, char* argv []){

    int stat = 0;
    int file = open (argv [1], O_RDONLY);
    if (file == -1){
        printf ("cannot open file\n");
        exit (1);
    }

    struct sigaction act;
    act.sa_handler = Handler;

    if ((stat = sigaction (SIGUSR1, &act, NULL)) < 0){
    
	    printf ("sigact SIGUSR1 err\n");
	    exit (1);
    }

    if ((stat = sigaction (SIGUSR2, &act, NULL)) < 0){

       	    printf ("sigact SIGUSR2 err\n");
	    exit (1);
    }

    if ((stat = sigaction (SIGTERM, &act, NULL)) < 0){

	    printf ("sigact SIGTERM err\n");
	    exit (1);
    } 

    if ((stat = sigaction (SIGCHLD, &act, NULL)) < 0){
    
	    printf ("sigact SIGCHLD err\n");
	    exit (1);
    }

    sigset_t mask;

    if ((stat = sigemptyset (&mask)) < 0){
     
	    printf ("sigempty err\n");
	    exit (1);
    }

    if ((stat = sigaddset (&mask, SIGUSR1)) < 0){
    
	    printf ("sigadd SIGUSR1 err\n");
	    exit (1);
    }

    if ((stat = sigaddset (&mask, SIGUSR2)) < 0){
	    
	    printf ("sigadd SIGUSR2 err\n");
	    exit (1);
    }

    if ((stat = sigprocmask (SIG_BLOCK, &mask, NULL)) < 0){
	    
	    printf ("sigproc mask err\n");
	    exit (1);
    }

    int parent_id = getpid ();
    
    int child_stat = fork ();
//in child proc
    if (child_stat == 0){

	    if (prctl (PR_SET_PDEATHSIG, SIGTERM) == -1){
		    
		    printf ("Daddy is dead\n");
		    exit (1);
	    }
	    sigset_t child_mask;

	    if ((stat = sigfillset (&child_mask)) < 0){
	    	 
		    printf ("sigfill child_mask err\n");
		    exit (1);
	    }
//	    if ((stat = sigdelset (&child_mask, SIGUSR1)) < 0){
//	    	
//	    	printf ("sigdel (child_mask) SIGUSR1 err\n");  
//	  	exit (1);
//	    }
//	    if ((stat = sigdelset (&child_mask, SIGTERM)) < 0){
//	    
//	    	printf ("sigdel (child_mask) SIGTERM err\n");
//	    	exit (1);
//	    }
	    char buffer = 0;

	    while (read (file, &buffer, 1) != 0){
	    	
		    for (int i = 0; i < 8; i++){
		    
			    int bt = ConvertBin (buffer, i);

			    if (bt == 0) kill (parent_id, SIGUSR1);
			    else kill (parent_id, SIGUSR2);

			    sigsuspend (&child_mask);	
		    }
	    }
    }
    //in parent proc
    else if (child_stat > 0){

	    sigset_t parent_mask;

	    if ((stat = sigfillset (&parent_mask)) < 0){
	     
		    printf ("sigfill parent_mask err\n");	    
		    exit (1);
	    }

	    if ((stat = sigdelset (&parent_mask, SIGUSR1)) < 0){
	     
		    printf ("sigdel (parent_mask) SIGUSR1 err\n");
		    exit (1);
	    }

	    if ((stat = sigdelset (&parent_mask, SIGUSR2)) < 0){
	     
		    printf ("sigdel (parent_mask) SIGUSR2 err\n");
		    exit (1);
	    }

//	    if ((stat = sigdelset (&parent_mask, SIGTERM)) < 0){
// 
// 		 printf ("sigdel (parent_mask) SIGTERM err\n");
//	   	 exit (1);
//	    }
//
//	    if ((stat = sigdelset (&parent_mask, SIGCHLD)) < 0){
//
//		 printf ("sigdel (parent_mask) SIGCHLD err\n");
//	    	 exit (1);
//	    }
//
//	    if ((stat = sigdelset (&parent_mask, SIGSTOP)) < 0){
//		 
//		 printf ("sigdel (parent_mask) SIGSTOP err\n");
//	  	 exit (1);
//	    }
//
	    char buffer = 0;

	    for (;;){
	    
		    buffer = 0;
		    for (int i = 0; i < 8; i++){
		    
			    if ((stat = sigsuspend (&parent_mask)) < 0){

				    printf ("sigsus parent_mask err\n");
				    exit (1);
			    }
			    kill (parent_id, SIGUSR1);
			    buffer = buffer | (bit << i);
		    }
		    write (1, &buffer, sizeof (char));
	    }
	    waitpid (parent_id, NULL, 0);
	    
	
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


int ConvertBin (char character, int number){
	
	if (number > 7 || number < 0) return -1;
	
	return ((character & (1 << number)) ? 1 : 0);
}
