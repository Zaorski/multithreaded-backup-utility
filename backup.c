//Thomas Zaorski
//EMAIL: Zaorst@rpi.edu
//RIN: 660747712

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

//Creates variables to count the total number of bytes and files copied
long long total_bytes = 0;
int total_files = 0;
//Sets to one if user enters the "-r" argument
int restore = 0;

//Sets up the mutex lock
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//Struct used as argument given to threads
struct arg_struct
{
	struct dirent *current;
	struct stat buf;
};

//Function threads execute
void * mybackup (void *arguments)
{
	//User is backing up instead of restoring
	if (restore == 0)
	{
	//Locks 
	pthread_mutex_lock( &mutex );
	struct arg_struct *args = arguments;
  	printf( "[THREAD %u] Backing up ", (unsigned int)pthread_self());
	printf( "%s...\n", args->current->d_name );
	
	//Increases the counter of number of bytes copied
	total_bytes += args->buf.st_size;
	
	char *st = args->current->d_name;
	char st1[100];
	strcpy(st1, st);
	char st2[100] = "./.mybackup/";
	strcat(st1, ".bak");
	strcat(st2, st1);

	//Checks if a backup file for current file already exists
	struct stat sti;
	if (stat (st2, &sti) == 0)
	{
		printf("[THREAD %u] WARNING: %s exists (overwriting!)\n", (unsigned int)pthread_self(),st1);
	}

	//Unlocks
	pthread_mutex_unlock( &mutex );

	//Opens file to be copied
	int fileread = open(st, O_RDONLY);

	//Allocates space
	void *buffer;
	buffer = malloc(sizeof(void) * args->buf.st_size);

	//Reads
	read(fileread,buffer,args->buf.st_size);

	//Creates backup file to be written to
	int filewrite = open(st2, O_CREAT | O_TRUNC | O_WRONLY, 0660);

	//Writes
	write(filewrite,buffer,args->buf.st_size);
	
	//Closes
	close(filewrite);
	close(fileread);
	printf( "[THREAD %u] Copied ", (unsigned int)pthread_self());
	printf( "%lld bytes from %s to %s\n", (long long)args->buf.st_size, st, st1 );

	//Increases number of files copied
	total_files++;
	}
	else //User is restoring current working directory with contents of .mybackup
	{
	//Locks
	pthread_mutex_lock( &mutex );
	struct arg_struct *args = arguments;

	
	char *str= args->current->d_name;
	  char new[100];
	char st[100];
	strcpy(st, str);

	//Strips the .bak off the file name
  	int size = (int)strlen(str);
  	strncpy(new, st, size - 4);

  	printf( "[THREAD %u] Restoring ", (unsigned int)pthread_self());
	printf( "%s...\n", new );

	total_bytes += args->buf.st_size;
	char st1[100] = "./.mybackup/";
	char st2[100] = "./";
	strcat(st1, st);
	strcat(st2, new);
	//Unlocks
	pthread_mutex_unlock( &mutex );
	
	//Opens file to be restored
	int fileread = open(st1, O_RDONLY);

	//Allocates space
	void *buffer;
	buffer = malloc(sizeof(void) * args->buf.st_size);

	//reads
	read(fileread,buffer,args->buf.st_size);

	//Creates restored file
	int filewrite = open(st2, O_CREAT | O_TRUNC | O_WRONLY, 0660);

	//Writes
	write(filewrite,buffer,args->buf.st_size);
	close(filewrite);
	close(fileread);

	printf( "[THREAD %u] Copied ", (unsigned int)pthread_self());
	printf( "%lld bytes from %s to %s\n", (long long)args->buf.st_size, str, new );

	total_files++;

	}

	return NULL;
} 

int main (int argc, char **argv)
{
    //If user provides "-r" argument, sets restore to 1
     if (argc == 2)
    {
	if ((strcmp(argv[1], "-r") == 0))
	{
	    restore = 1;
	}
	else
	{
		printf("Invalid Command\n");
		return EXIT_FAILURE;
	}
    }
    
	//Checks if hidden file .mybackup does not exist. Creates it if it does not
	struct stat st;
	if (stat("./.mybackup", &st) != 0)
	{
		mkdir("./.mybackup", S_IRWXU | S_IRWXG | S_IRWXO);
	}

	//Children is the number of child threads
	int children = 0;
	int thread, i;
	//Array of threads
	pthread_t tid[100];

	//By default we are dealing with the current working directory
	char * current_path = ".";
	//If restore is set to 1 set the directory to be the .mybackup folder (used in Part 2)
	if (restore == 1)
        {
	  current_path = "./.mybackup";
	}
	
	DIR * dir = opendir((char*)current_path); //Opens the current directory
	if (dir == NULL)  //Detects failure to open directory
	{
	    perror("opendir() Failed");
	    return EXIT_FAILURE;
	}

	struct dirent *current; 
	int rc = 0;

	//readdir sets current = to the items in the directory one at a time until none are found (NULL)
	
	while ((current = readdir(dir)) != NULL)
	{
	    struct stat buf;
	    char new[10000]; //Used to create the complete path name
	
	    //Next few lines create the complete path name required for Stat()
	    strcpy(new, current_path);
	    strcat(new, "/");
	    strcat(new, (char*)current->d_name);
	    rc = stat(new, &buf);
	    //rc = stat(current -> d_name, &buf);
	    if (rc < 0) //Detects stat() failure
	    {
		perror("stat() Failed");
		return 1;
	    }
	    else
	    {
		//Only bothers with regular files
		if (S_ISREG(buf.st_mode)) //If directory entry is a regular file
		{
			struct arg_struct *args;
			//Allocates space for the thread argument
			args = (struct arg_struct *)malloc(sizeof(struct arg_struct));
			args->current = current;
			args->buf = buf;
			//Creates thread
			thread = pthread_create(&tid[children], NULL, mybackup, (void *)args);
			 if ( thread != 0 )
    			{
      				perror( "MAIN: Could not create child thread" );
    			}
			children++;
			
		}
	    }
		
	}
	//Waits for all threads to finish
	for (i = 0; i < children; i++)
	{
		pthread_join(tid[i], NULL);
	}
	if (restore == 0)
		printf("Successfully backed up %d files (%lld bytes)\n", total_files, total_bytes);
	else
		printf("Successfully restored %d files (%lld bytes)\n", total_files, total_bytes);		
   
return 0;
}
