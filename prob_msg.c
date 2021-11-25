#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <sys/ipc.h>



struct my_message {

    long type;
};

int main (int argc, char* argv[]){

    //scanning num of chilrens
    char* endptr = NULL;

    long num_of_child = strtol (argv[1], &endptr, 10);

    //create msq

    if (*endptr == 0 && num_of_child > 0){

        int msq_id = msgget (IPC_PRIVATE, IPC_CREAT | 0666);

        if (msq_id < 0){

            printf ("Cannot create msq! msgget error\n");
            exit(1);
        }

    //born child

        pid_t pid = 0;

        for (long number = 1; number <= num_of_child; number++){

            pid = fork ();

            if (pid == -1){

                printf ("Cannot born child\n");
                exit(1);
            }
    //in child process
            else if (pid == 0){

                struct my_message msg;

    //recive messefe from daddy
                if (msgrcv (msq_id, &msg, sizeof (msg) - sizeof (long), number, MSG_NOERROR) == -1){

                    printf ("cannot recive message from daddy, child #%ld\n", number);
                    exit(1);
                } 

                printf("I`m child #%ld\n", number);
                //fflush(0);
    //send message to daddy
                msg.type = num_of_child + 1;

                if (msgsnd (msq_id, &msg, sizeof (msg) - sizeof (long), MSG_NOERROR) == -1){

                    printf ("cannot send message for daddy, child #%ld\n", number);
                    exit(1);
                }
                return 0;
            }
        }
    //in parent process 

        for (long number = 1; number <= num_of_child; number++){

            struct my_message msg;

            msg.type = number;
    //send message to child #number
            if (msgsnd (msq_id, &msg, sizeof (msg) - sizeof (long), MSG_NOERROR) == -1){

                printf ("cannot send message to child #%ld\n", number);
                exit (1);
            }
    //recive message from child #number
            if (msgrcv (msq_id, &msg, sizeof (msg) - sizeof (long), num_of_child + 1, MSG_NOERROR) == -1){

                printf ("cannot recive message from child #%ld\n", number);
                exit (1);
            }
    //check correctly reciving messages from childrens
            if (msgctl (msq_id, IPC_RMID, NULL) != 0){

                printf ("we have a problem, msgctl () error\n");
                exit (1);
            }
            //else printf ("oh shit , I`m sorry\n");
	}
    }

}
