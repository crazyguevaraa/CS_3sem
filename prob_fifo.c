#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <assert.h>
 

int main (int argc, char* argv[]){

	int size;

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

			
		//
		//connect to global FIFO
		//

		if ((fd_global_fifo = open (global_name, O_WRONLY)) == -1){
			
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
		//write a local name in global FIFO
		//
		
		if ((write (fd_global_fifo, uniq_name, uniq_name_size))){

			printf ("Cannot writing uniq name\n");
			exit (1);
		}
		
		else printf("uniq name is OK\n");	
		/*//
		//create local FIFO
		//

		int local_status = mknod (uniq_name, S_IFIFO | 0666, 0);
		if (local_status < 0){
			
			printf ("Cannot create local FIFO\n");
			exit (1);
		}*/
		sleep(10);

//
//тут при смерти записывающего процесса процесса
//читающий на веки вечные становится в ожидании в open()
//

		//
		//connect to local fifo
		//

		if ((fd_local_fifo = open (uniq_name, O_WRONLY)) == -1){
			
			printf ("Cannot connect to local FIFO\n");
			exit (1);
		}

		//
		//copy file to local FIFO
		//

		
		int file_status = 0;

		if ((file_status = open (argv[1], O_RDONLY)) == -1){
				
			printf ("Cannot open FILE\n");
			exit (1);
		}	
		
		for (;;){
			
			if ((size = read (file_status, buffer, buffer_size)) && (write (fd_local_fifo, buffer, size)))
				continue;
		
			else {
				close (file_status);
				break;
		
			}
		}

		}		
	/////////////////////////////////////////////
	// part of programm, that reads to FIFO
	/////////////////////////////////////////////

	else {
		
		///
		//connect to global FIFO
		//

		if ((fd_global_fifo = open (global_name, O_RDONLY)) == -1){
			
			printf ("Cannot connect to group\n");
			exit (1);
		}

 		//
		//read a local name in global FIFO
		//
		
		if ((read (fd_global_fifo, uniq_name, uniq_name_size)) < 0){

			printf ("Cannot reading uniq name\n");
			exit (1);
		}
		
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


