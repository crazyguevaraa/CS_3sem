#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <assert.h>
 

int main (int argc, char* argv[]){

	int size = 0;
	int status = 0;

	int fd_global_fifo = 0;
	int fd_local_fifo  = 0;
	const int buffer_size    = 10;
	const int uniq_name_size = 12;	
	char uniq_name[uniq_name_size];
	char buffer[buffer_size]; 

	const char* global_name = "a.fifo";

	(void) umask (0);
	
	//
	//create global FIFO
	//
	
	int global_status = mknod (global_name, S_IFIFO | 0666, 0);
	if (global_status < 0){

		printf ("Cannot create global FIFO\n");
	}



	/////////////////////////////////////////////
	//part of programm, that writes to FIFO
	/////////////////////////////////////////////

	if (argc > 1 && argc < 3){

		int file_status = 0;

		if ((file_status = open (argv[1], O_RDONLY)) == -1){
				
			printf ("Cannot open FILE\n");
			exit (1);
		}	

		//
		//connect to global FIFO
		//

		if ((fd_global_fifo = open (global_name, O_RDONLY)) == -1){
			
			printf ("Cannot connect to group\n");
			exit (1);
		}
		//
		//read a local name in global FIFO
		//
		
		if ((read (fd_global_fifo, uniq_name, uniq_name_size)) != uniq_name_size){

			printf ("Cannot reading uniq name\n");
			exit (1);
		}
		//
		//connect to local fifo
		//

		if ((fd_local_fifo = open (uniq_name, O_WRONLY | O_NONBLOCK)) == -1){
			
			printf ("Cannot connect to local FIFO\n");
			exit (1);
		}

		close (fd_global_fifo);

		if ((status = fcntl (fd_local_fifo, F_SETFL, O_WRONLY)) == -1){
			
			printf ("Cannot update local fifo\n");
			exit (1);
		}

		//
		//copy file to local FIFO
		//
		
		for (;;){
			
			if ((size = read (file_status, buffer, buffer_size)) && (write (fd_local_fifo, buffer, size)))
				continue;
		
			else {
				close (file_status);
				break;
		
			}
		}
			
		unlink (uniq_name);
		close (file_status);
	}
		
	/////////////////////////////////////////////
	// part of programm, that reads to FIFO
	/////////////////////////////////////////////

	else {
		int fd_global_fifo_2;
		///
		//connect to global FIFO
		//

		if ((fd_global_fifo = open (global_name, O_WRONLY)) == -1){
			
			printf ("Cannot connect to group\n");
			exit (1);
		}
		if ((fd_global_fifo_2 = open (global_name, O_WRONLY)) == -1){
			
			printf ("Cannot connect to group\n");
			exit (1);
		}

		//
		//create uniq name of local FIFO
		//
		
		int pid = getpid();
			
		int local_name_status = sprintf (uniq_name, "local%d", pid);

		assert (local_name_status);

		//
		//create local FIFO
		//

		int local_status = mknod (uniq_name, S_IFIFO | 0666, 0);
		if (local_status < 0){
			
			printf ("Cannot create local FIFO\n");
			exit (1);
		}

		//
		//connect to local fifo
		//
		
		if ((fd_local_fifo = open (uniq_name, O_RDONLY|O_NONBLOCK)) == -1){
			
			printf ("Cannot connect to local FIFO\n");
			exit (1);
		}
		
		//
		//write a local name in global FIFO
		//
		
		if ((write (fd_global_fifo, uniq_name, uniq_name_size)) <= 0){

			printf ("Cannot writing uniq name\n");
			exit (1);
		}
		
		else printf("uniq name is OK\n");
		
		close (fd_global_fifo);

		if ((status = fcntl (fd_local_fifo, F_SETFL, O_RDONLY)) == -1){
		
			printf ("Cannot update local fifo (read)\n");
			exit (1);
		}
		
		sleep (10);

		//
		//reading a file from local FIFO
		//		
		
		for(;;){	
			if ((size = read (fd_local_fifo, buffer, buffer_size))){
				
				write (1, buffer, size);
			}

			else break;

		}
		
	
	}

        unlink (uniq_name);
	unlink (global_name);
}


