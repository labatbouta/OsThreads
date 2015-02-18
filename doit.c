#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <stdbool.h>

struct jobDescription
{
	int pid;
	char *stripped_command[128];
};

/* calculates the usage time of the user and system cpu, the number of page faults, 
number of page faults that can be satisfied by reclaiming memory, 
preempted proceses, and processes that give up and print them out*/
void recieveStatistics(struct timeval start)
{
struct timeval end;


/*retreive wall clock end time*/
gettimeofday(&end, NULL);

    /* CPU user and system times*/
    struct rusage ru;
    struct timeval utime; // user cpu time
    struct timeval stime; // system cpu time

    /* page fault and other processes fault numbers*/
    int pagefaults; 
    int page_faults_reclaim;
    int processes_preempted;
    int processes_give_up;


    getrusage(RUSAGE_CHILDREN, &ru);
    utime = ru.ru_utime;
    stime = ru.ru_stime;
    pagefaults = ru.ru_majflt;;
    page_faults_reclaim = ru.ru_minflt;
    processes_preempted = ru.ru_nivcsw;
    processes_give_up = ru.ru_nvcsw;


	printf( "Time User CPU: %03ld ms\n", (utime.tv_usec/(long)1000));
    printf("CPU System time: %03ld ms\n", (stime.tv_usec/(long)1000));
    printf("Number of Page Faults: %d \n", pagefaults);
    printf("Number of Page Faults that can be satisfied by reclaiming memory: %d \n", page_faults_reclaim);
    printf(" Number of Processes Preempted involuntarily: %d\n", processes_preempted );
    printf("Number of Time Processes Give Up: %d\n", processes_give_up );
	printf(" Wall-clock time: %03ld ms \n", ((end.tv_usec/1000) - ( start.tv_usec/(long)1000)));
}

/* takes in the given commands from user and the amount of commands
and determines if task will run in background*/
bool isBackground(char *shell_args[33], int argcount){
	 char stringAMP[1]="&";
	if( strncmp(stringAMP, shell_args[argcount-1], 1) == 0){
		return true;
	}
	
		return false;
}

/* checks the pid and forks the parent if the fork process has not been done*/
void createFork(int argc, char **argv, char *shell_args[33], int argcount, int cd_present)
{
  int pid; // process id 
  struct timeval start; // start time of process
 int background_process_id;


  /* retreive wall clock start time*/
  gettimeofday(&start, NULL);


  if ((pid = fork()) < 0) 
  {
        fprintf(stderr, "Fork error\n");
        exit(1);
    }
    else if (pid == 0) 
    {

      // Create file input parameter for the execvp function
      // Shell commands are in the bin directory and then append the actual
      // command that the user specified (in argv[1]) plus null terminator.
     char  shell_command[128] = "./";
      char * word = strtok (argv[1], " ");
      strcat(shell_command, word);
      background_process_id = getpid();
      /* Build the list of inputs to this shell command (if any specified).*/
      char * newargv[32] = { shell_command};
      int i;
      for (i=2; i < argc; i++)
      {
    	newargv[i-1] = argv[i];
      }
      	newargv[argc-1] = NULL;  /* terminate shell argument list with NULL */

      printf("Shell command is %s\n ", shell_command);
      if (execvp(shell_command, newargv) < 0) 
      {
        fprintf(stderr, "Execvp error\n");
        perror("Execvp is failing with this error");
        exit(1);
        }
    }

    else 
    {
        /* parent */


    wait(0);        /* wait for the child to finish */ 

    if(cd_present != 1)	{
     recieveStatistics(start); /* print statistics in terminal*/
}

    }
}



int main(int argc, char **argv)
    /* argc -- number of arguments */
    /* argv -- an array of strings */
{

   char line[128]; // input
        const char s[2] = " ";
        char *token;
        char *saved_token;
        char *last_char;
        int  cd_present; 
        int  bad; // check to see if bad command (errror)
        char string1[10] = "exit";
        char string2[2] = "cd";
        char string0[1] = "\n";
        char string_job[4] = "jobs";
        int argcount;         
        char *shell_args[33];
        int   i, status, next_index;

   /* user entered arguments to command line*/
   if(argc > 1)
   {
   		// create new child
       createFork(argc, argv, shell_args, argcount, cd_present);
       exit(0);  /* Success */
   }
   else
    {
        

        /* Create the shell argument list */
        for (i=1; i<33; i++)
    {
             shell_args[i] = malloc(32); // allocate memory
        }

        while (1)
        {

      /* Initialize the shell argument list */
      for (i=1; i<33; i++)
      {
        strncpy(shell_args[1]," ", 32);
          }
           next_index = 1; // next argument
           argcount = 1; // argument counter
           cd_present = 0; // is cd current command
           bad = 0; // error in message entered

            /* Print the command prompt */
            printf("==> ");
            fflush(NULL);

            /* Read a command line */
            if (!fgets(line, 128, stdin))
            {
               return 0;
            }

            //If no data was entered, exit the program.
            if(line[0] == EOF)
            {
               exit(0);
            }

           

            /* get the first token */
            token = strtok(line, s);
            saved_token = token;

            /* if no input is given*/
 		if(strncmp(token, string0, strlen(token)) == 0)
        {
          continue; 
            }

            /* If exit, leave.*/
            if (strncmp(token, string1, (strlen(token)-1)) == 0)
          {
        exit(0);                // Treat exit command special.
              }

              /* If user calls "jobs" in shell*/
              if(strncmp(token, string_job, (strlen(token)-1)) == 0){
              	printf("This should print out any open background tasks and the corresponding pid\n");
              }

/* if user calls cd*/
            if (strncmp(token, string2, strlen(token)) == 0)
          {
        cd_present=1;                // cd command is special too.
              }
                 
                 /* If no input, then skip processing and give new prompt. */
      			/* otherwise read other input*/
            strncpy(shell_args[next_index], token, strlen(token));
            argcount++;
            next_index++;

            /* walk through other tokens */
            while( (token = strtok(NULL, s)) != NULL ) 
            {
              saved_token = token;
              argcount++;
              
              /* if cd is present change directory*/  
              if (cd_present == 1 )
          {
                    char directory[1024];
    
                    strncpy(directory, token, strlen(token)-1); // change directory to token

                    /* if false directory tell user*/
                    if(chdir(directory) != 0) 
                    {
                       fprintf(stderr, "Change directory error\n");
                       perror("Change directory is failing with this error");
                       bad = 1;
                    }
          }

              else
          {
                   strcpy(shell_args[next_index], token);
                   next_index++;
              }
            }    /* end of while token != null */
            
            if (cd_present != 1 && bad != 1)
        {
               /* Need to remove the new line character at the end of the last token. */
           last_char = shell_args[next_index-1] + strlen(saved_token)-1 ;
               strncpy(last_char, "", 1);
               if(!isBackground(shell_args, argcount)){
               /* Now ready to execute the shell command */
               createFork(argcount,(char **)&shell_args, shell_args, argcount, cd_present);
           }
           else
           	{ 
           		printf("This is a background task and there should be no wait\n");
           		printf("Background Task: %s\n", token);
       			}
        }
    }   /* end of while (1)*/

        /* Free the memory allocated for the arg list. */
        for (i=1; i<33; i++)
    {
      free(shell_args[i]);
        }

    } /* end of else part of if (argc >1) */

}
