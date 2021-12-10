#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>

#define MAX(a,b) ((a > b) ? a : b)
#define MIN(a ,b) ((a < b) ? a : b)

struct circle_buff_t {

    char *p_buff;

    size_t ri;
    size_t wi;
    size_t size;
};

typedef struct circle_buff_t* circle_buff;

struct chan_inf {

    int fd_from;
    int fd_to;

    circle_buff buf;
};

enum CIRCLE_BUFF_RES {

    BUF_SUCCESS = 0,
    BUF_FAILURE = -1

};

int    create_circle_buff (size_t size, circle_buff *buffer);
int    destroy_circle_buff (circle_buff buffer);
int    circle_buffer_fd_reader (circle_buff buffer, int fd, size_t num, int *read_res);
int    circle_buffer_fd_writter (circle_buff buffer, int fd, size_t num, int *write_res);
size_t circle_buffer_get_size (circle_buff buffer);
int    circle_buffer_is_empty (circle_buff buffer);
int    circle_buffer_is_full (circle_buff buffer);
void   child_handler (int signo);
void   free_all (struct chan_inf *arr, size_t num);


int main (int argc, char* argv[]){
    
    errno = 0;
    char* file_name = argv[1];
    char* cur_end = NULL;

    long num_of_child = strtol (argv[2], &cur_end, 10);
    if (*cur_end != 0 || num_of_child <= 0){

        printf ("incorrect num_of_child\n");
        exit (1);
    }

    struct sigaction act = {};
    act.sa_handler = child_handler;

    sigaction (SIGCHLD, &act, NULL);

    struct chan_inf *chan_inf = (struct chan_inf*) calloc (num_of_child, sizeof (struct chan_inf));
    assert (chan_inf != NULL);

    for (long i = 0; i < num_of_child; i++){

        int fd_ptr_to[2] = {};
        int fd_ptr_from[2] = {};
        pipe (fd_ptr_to);
        pipe (fd_ptr_from);

        pid_t parent_id = getpid ();
        pid_t child_id = fork ();

        if (child_id < 0){

            printf ("err: fork() #%ld\n", i);
            exit (1);
        }
        //
        //in child proc
        //
        if (child_id == 0){

            for (int j = 0; j < i; ++j){

                close (chan_inf[j].fd_from);
                close (chan_inf[j].fd_to);
            }

            if (prctl (PR_SET_PDEATHSIG, SIGTERM) == -1){

                printf ("err(child): daddy is dead\n");
                exit (1);
            }

            if (parent_id != getppid ()){

                printf ("err(child): daddy is really dead\n");
                exit (1);
            }

            int fd_to = -1;
            int fd_from = -1;
            //
            //in first child
            //
            if (i == 0){

                close (fd_ptr_to[0]);
                close (fd_ptr_to[1]);
                close (fd_ptr_from[0]);

                fd_to = fd_ptr_from[1];
                fd_from = open (file_name, O_RDONLY);
                if (fd_from <= 0){

                    printf ("err(child): cannot open file %s\n", file_name);
                    exit (1);
                }
            }
            //
            //in othere child
            //
            else {

                close (fd_ptr_from[0]);
                close (fd_ptr_to[1]);

                fd_to = fd_ptr_from[1];
                fd_from = fd_ptr_to[0];
            }
            //
            //traansfer resourse in child
            //
            ssize_t splice_result = 0;
            do {

                splice_result = splice (fd_from, NULL, fd_to, NULL, 10000, SPLICE_F_MOVE);
            } while (splice_result > 0);

            if (splice_result < 0) {

                printf ("err(child): splice\n");
                exit (1);
            }

            close (fd_to);
            close (fd_from);

            return 0;
        }
    
        //
        //in parent proc
        //
        else{

            if (create_circle_buff ((size_t)(pow (3, num_of_child - i) * 1024), &(chan_inf[i].buf)) != BUF_SUCCESS){

                printf ("err(parent): cannot create buffer\n");
                exit (1);
            }

            chan_inf[i].fd_to = -1;
            chan_inf[i].fd_from = -1;

            close (fd_ptr_to[0]);
            close (fd_ptr_from[1]);

            //
            //for first child
            //
            if (i == 0){

                close (fd_ptr_to[1]);

                chan_inf[0].fd_from = fd_ptr_from[0];

                if (num_of_child == 1){
                
                   chan_inf[0].fd_to = STDOUT_FILENO;
                }
            }

            else {

                chan_inf[i - 1].fd_to = fd_ptr_to[1];
                chan_inf[i].fd_from = fd_ptr_from[0];

                if (i == num_of_child - 1){

                    chan_inf[i].fd_to = STDOUT_FILENO;
                }
            }
        } 
    }

    for (long i = 0; i < num_of_child; i++){

        int result = fcntl (chan_inf[i].fd_from, F_SETFL, O_NONBLOCK);
        
        if (result < 0){

            printf ("err: fcntl() for read #%ld\n", i);
            exit (1);
        }

        result = fcntl (chan_inf[i].fd_to, F_SETFL, O_NONBLOCK);

        if (result < 0){

            printf ("err: fcntl() for write #%ld\n", i);
            exit (1);
        }
    }

    fd_set rfds = {};
    fd_set wfds = {};

    long alive = 0;

    while (chan_inf[num_of_child - 1].fd_from >= 0){

        FD_ZERO (&rfds);
        FD_ZERO (&wfds);

        int num_select = -1;

        for (int i = 0; i < num_of_child; i++){

            if (chan_inf[i].fd_from >= 0 && !circle_buffer_is_full (chan_inf[i].buf)){

                FD_SET (chan_inf[i].fd_from, &rfds);

                num_select = MAX(num_select, chan_inf[i].fd_from);
            }

            if (chan_inf[i].fd_to >= 0 && !circle_buffer_is_empty (chan_inf[i].buf)){

                FD_SET (chan_inf[i].fd_to, &wfds);

                num_select = MAX(num_select, chan_inf[i].fd_to);
            }
        }

        if (select (num_select + 1, &rfds, &wfds, NULL, NULL) < 0){

            printf ("err: select()\n");
            exit (1);
        }

        for (long i = alive; i < num_of_child; i++){

            if (FD_ISSET (chan_inf[i].fd_from, &rfds)){

                int read_result = 0;

                if (circle_buffer_fd_reader (chan_inf[i].buf, chan_inf[i].fd_from, circle_buffer_get_size (chan_inf[i].buf), &read_result) != BUF_SUCCESS){
                    
                    printf ("err: read\n");
                    exit (1);
                }

                if (read_result < 0){

                    printf ("err: parent read\n");
                    exit (1);
                }

                if (read_result == 0){

                    close (chan_inf[i].fd_from);
                    chan_inf[i].fd_from = -1;
                }
            }

            if (FD_ISSET (chan_inf[i].fd_to, &wfds)){

                int write_result = 0;
                if (circle_buffer_fd_writter (chan_inf[i].buf, chan_inf[i].fd_to, circle_buffer_get_size(chan_inf[i].buf), &write_result) != BUF_SUCCESS){

                    printf ("err: write\n");
                    exit (1);
                }

                if (write_result < 0){

                    printf ("err: write\n");
                    exit (1);
                }
            }

            if (chan_inf[i].fd_from < 0 && circle_buffer_is_empty (chan_inf[i].buf)){

                if (i != alive){

                    printf ("err: some child are dead\n");
                    exit (1);
                }

                close (chan_inf[i].fd_to);
                chan_inf[i].fd_to = -1;

                alive++;
            }
        }
    }

    free_all (chan_inf, num_of_child);

    return 0;
}

int create_circle_buff (size_t size, circle_buff *buffer){

    *buffer = (circle_buff) calloc (1, sizeof (struct circle_buff_t));
    if (buffer == NULL){

        return BUF_FAILURE;
    }
    
    circle_buff tmp = *buffer;

    tmp -> p_buff = (char*) calloc (size , sizeof (char));
    if (tmp -> p_buff == NULL){

        return BUF_FAILURE;
    }

    tmp -> size = size;
    tmp -> ri = 0;
    tmp -> wi = 0;

    return BUF_SUCCESS;
}

int destroy_circle_buff (circle_buff buffer){

    if (buffer == NULL){

        return BUF_FAILURE;
    }

    free (buffer -> p_buff);
    buffer -> p_buff = NULL;
    buffer -> size = 0;
    buffer -> ri = 0;
    buffer -> wi = 0;
    free (buffer);

    return BUF_SUCCESS;
}

int circle_buffer_fd_reader (circle_buff buffer, int fd, size_t num, int *read_res){

    if (buffer == NULL || buffer -> p_buff ==NULL){

        return BUF_FAILURE;
    }

    size_t buf_size = buffer -> size;
    int result = 0;

    result = read (fd, buffer -> p_buff + (buffer -> wi % buf_size), MIN(buf_size - (buffer -> wi % buf_size), buf_size - (buffer -> wi - buffer -> ri)));

    if (result > 0){

        buffer -> wi += result;
    }

    if (result != NULL){

        *read_res = result;
    }

    return BUF_SUCCESS;
}

int circle_buffer_fd_writter (circle_buff buffer, int fd, size_t num, int *write_res){

    if (buffer == NULL || buffer -> p_buff == NULL){
        
        return BUF_FAILURE;
    }

    size_t buf_size = buffer ->size;
    int result = 0;

    result = write (fd, buffer -> p_buff + (buffer -> ri % buf_size), MIN(buffer -> wi - buffer -> ri, buf_size - (buffer -> ri % buf_size)));

    if (result > 0){
        
        buffer -> ri += result;
    }

    if (write_res != NULL){

        *write_res = result;
    }

    return BUF_SUCCESS;
}

size_t circle_buffer_get_size (circle_buff buffer){

    return buffer -> size;
}

int circle_buffer_is_empty (circle_buff buffer){

    return buffer -> wi == buffer -> ri;
}

int circle_buffer_is_full (circle_buff buffer){

    return buffer -> wi - buffer -> ri == buffer -> size;
}

void child_handler (int signo){

    if (signo == SIGCHLD){
        
        int stat = 0;
        wait (&stat);

        if (stat != 0){

            printf ("one of child is died\n");
            exit (1);
        }
    }
}

void free_all (struct chan_inf *arr, size_t num){

    for (size_t i = 0; i < num; i++){

        destroy_circle_buff (arr[i].buf);

        arr[i].buf = NULL;
    }

    free (arr);
}
