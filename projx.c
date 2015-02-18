// This program demonstrates the following:
//  1. maps a file into memory  
//  2  distributes the work among worker threads 
//  3 determines the number of strings in the file that are greater than four 
//  4 determines the maximum string length in file
//
// by Lisa Batbouta

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#define DEFAULT 1024
#define MAXTHREADS 16
#define RANGE 1
#define ALLDONE 2

/* message structure 
Messages are passed back and forth between 
the main thread and the worker threads.  */
struct msg{
  int iFrom; //the thread that is sending the message (the main thread is thread 0)
  int type; //can be either RANGE (sent from the main thread) or ALLDONE (from a worker thread)
  int value1;
  int value2;
  int firstCharStringLen;
  int lastCharStringLen;
};

/* global semaphores and threads */
sem_t threads[MAXTHREADS+1]; // array of semaphores for each thread
pthread_t idthreads[MAXTHREADS+1]; // array of thread pids
struct msg mailboxes[MAXTHREADS+1];// array of mailboxes, messages are stored here
struct msg message_all_done[MAXTHREADS+1]; // array of messages transfered from one thread to main thread or vice versa
char *pchFile;
char *buf;
int count_w_threads;

/* prototype */
static unsigned get_file_size(const char * filename);
int number_of_strings_length_more_than_four(int start, int end, int *search_max);
int strings_length_more_than_four_w_thread(int start, int end, int *search_max, int *start_w_printable, int *end_w_printable);
void initialize_threads(int number_of_threads, int fileSize, int *search_max);
void *producer(int number_of_threads, int fileSize, int *search_max);
void * consumer(void *arg);


 // Main processing.  Input arguments are :
     //
     //   1st argument:   File to be processed
     //   2nd argument:  file size of chunks of data to be proceesed at a time, 
     //                   mmap (map to memory), or "pX" where X is the number of worker threads
     //                   that will calculate the number of strings in the file. Default chunk size is 1024 if no number is specifed
     //                    
int main(int argc, char *argv[]){
    int fd;// file descriptor
    struct stat sb;
    unsigned fileSize = 0;
    int number_of_strings = 0;
    int search_for_max = 0;
    int chunk_size;
    char mmapx[] = "mmap";
    int start = 0;
    bool mmap_bool = false;
    bool read_size = false;
    bool threads_bool = false;
    char letter_p[] = "p";
    char argv2[3];
    int num_threads = 0;

    // if there are less than 2 arguments given
    if(argc < 2) 
    {
      fprintf(stderr, "Usage: %s <input>\n", argv[0]);
      exit(1);
    }
   
      fprintf(stderr, "Usage: %s <%s>\n", argv[0], argv[1]);
      fileSize = get_file_size(argv[1]);
      printf("File size:%d\n", fileSize);
    
    // if there are 2 arguments give by user read file into buffer
     if(argc == 2)
    {  
      buf = malloc(fileSize);
      chunk_size = 1024; 
      read_size = true; 
    }

     // if there are three arguments given identify if to use mmap 
     // choose chunk size of each buffer
     // or thread program
    if(argc == 3)
    {
        if(strcmp(argv[2], mmapx) == 0)
        {
          mmap_bool = true;
        }
        else if(strncmp(argv[2], letter_p, 1) == 0)
        {
          strcpy(argv2, argv[2]+1);
          int size_of =0;     
          num_threads = atoi( argv2);
          if(num_threads > 16)
          {
            perror("must be no more than 16 threads\n");
            exit(1);
          }
          mmap_bool = true;
          threads_bool = true;
        }
        else
        {
          chunk_size = atoi(argv[2]);
          read_size = true;
          if(chunk_size > 8192)
          {
            perror("Chunk size cannot be greater than 8192\n");  
            exit(1);
          }
        }
        buf = malloc(fileSize);
    }

     
      /* Map the input file into memory */
    if ((fd = open (argv[1], O_RDONLY)) < 0) 
    {
      perror("Could not open file");
      exit (1);
    }

    // if user states to read file from buffer
    if(read_size)
    { 
      // while the start value is less than the file size
      // read file into buffer
       while(start < fileSize)
       {
          if(fileSize-start > fileSize)
          {
             read(fd,buf,chunk_size);
          }
          else
	        {
             read(fd,buf, (fileSize - start));
          }
          start+=chunk_size;
       }
        
       // calculate the max string length and the number of strings greater than 4
       number_of_strings = number_of_strings_length_more_than_four(0, fileSize-1, &search_for_max);
       printf("Strings greater than 4: %d\n", number_of_strings);   
       printf("Maximum string length is: %d\n", search_for_max);
    }

    // if user indicates to map file to memory and then read from the buffer
    if(mmap_bool)
    {
      // find the file size
         if(fstat(fd, &sb) < 0)
         {
             perror("Could not stat file to obtain its size");
             exit(1);
         }

         // map file to memory
         if ((pchFile = (char *) mmap (NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0)) 
              == (char *) -1)	
         {
              perror("Could not mmap file");
              exit (1);
         }

         buf = pchFile;

         // if user does not indicate to use threads
         if(!threads_bool)
         {
             // pchFile is now a pointer to the entire contents of the file...
             // processing of the file contents here
             number_of_strings = number_of_strings_length_more_than_four(0, fileSize-1, &search_for_max);
             printf("Strings greater than 4: %d\n", number_of_strings);
         }
         else
	 {  // threads_bool is true
            initialize_threads(num_threads, fileSize, &search_for_max); 
            printf("Strings greater than 4: %d\n", count_w_threads);
            fflush(stdout);
         }
 	  printf("Maximum string length is: %d\n", search_for_max);
          fflush(stdout);

         // Now clean up
         if(munmap(pchFile, sb.st_size) < 0)
         {
             perror("Could not unmap memory");
             exit(1);
         }
    }
    close(fd);
}   // End of main

/* gets the file size given the name of the file that you are trying to get
 * the size of
 */
static unsigned get_file_size(const char * filename)
{
struct stat sb;

if(stat(filename, &sb)!=0)
    {
      fprintf(stderr,"'stat' failed for '%s': %s.\n", filename, strerror(errno));
      exit(EXIT_FAILURE);
    }
return sb.st_size;
}

/* determines the number of strings that have a length greater than 4 and the maximum, 
 * string length of the strings in that buffer region
 * int start - the start value of the buffer
 * int end - the end value of the buffer
 * int * search max- searches buffered regoin for largest string length
 * returns - the number of strings that have a length greater than 4
 */
int number_of_strings_length_more_than_four(int start, int end, int *search_max)
{ int i;
  int counter = 0;
  int count_num_Characters = 0;
  bool is_first_time = true;

  for(i = start; i <=end; i++)
  {
    if(isprint(buf[i])|| isspace(buf[i]))
    {
      if(is_first_time)
      {
         counter ++;
         is_first_time = false;
      }
      count_num_Characters ++;

      //  check to see if current value of search max is still the largest
      if(count_num_Characters  > *search_max)
      {
         *search_max = count_num_Characters;
      }
    }
    else
      {   // non printable character.

       if(count_num_Characters < 4 && counter > 0 && (is_first_time == false))
       {
            counter -=1;
       }
 
      is_first_time = true;
      count_num_Characters = 0;
    }
  }
  return counter;
}

/* determines the number of strings that have a length greater than 4 and the maximum, 
 * string length of the strings in that buffer region using threads to distribute the work
 * int start - the start value of the buffer
 * int end - the end value of the buffer
 * int * search max- searches buffered regoin for largest string length
 * int *start_w_printable - gets length of printable at the beginning if starts with printable character
 * int *end_w_printable - gets the length of the last string in the line if ends with printable
 * returns - the number of strings that have a length greater than 4
 */

int strings_length_more_than_four_w_thread(int start, int end, int *search_max, int *start_w_printable, int *end_w_printable)
{
    int i;
    int counter = 0;
    int count_num_Characters = 0;
    bool is_first_time = true;
    bool is_first_char_string = false;
    bool process_string = false;

    if(isprint(buf[start]) || isspace(buf[start]))
    {
        *start_w_printable +=1;
        is_first_char_string = true;
    }

    if(isprint(buf[end]) || isspace(buf[end]))
    {
        *end_w_printable +=1;
    }

    for(i = start; i <=end; i++)
    {
       if(isprint(buf[i])|| isspace(buf[i]))
       {
           if(is_first_time)
           {
	   
              counter ++;
              is_first_time = false;
              process_string = true;
           }
           count_num_Characters ++;
            
           // Is the current count greater than our largest string size?
           if(count_num_Characters  > *search_max)
           {
               *search_max = count_num_Characters;
           }

           // If last character is printable, return size of this string.
           if (i == end)
	   {
	     *end_w_printable = count_num_Characters;
              if (is_first_char_string)
	      {
	         *start_w_printable = count_num_Characters;
                 is_first_char_string = false;
              }
	   }
       }
       else
       {
	      // Non printable character
     
           if(count_num_Characters < 4 && counter > 0 && !is_first_char_string  && process_string)
           {
              counter -=1;
           }
 
           if (is_first_char_string)
	          {
	            *start_w_printable = count_num_Characters;
             is_first_char_string = false;
            }
           is_first_time = true;
           process_string = false;
           count_num_Characters = 0;   // reset
       }
    }
    return counter;
}


/* Creates threads to be used to calculate the maximum number of threads
 * and the string count greater than 4 in length
 * int number_of_threads - is the number of threads to use given by the user
 * int fileSize - is the size of the file to be mapped
 * int *search_max takes in a max value of 0 and tries to find values greater than
 * that in the given buffered regoin
 */
void initialize_threads(int number_of_threads, int fileSize, int *search_max)
{
  int i;

  *search_max = 0;

  // set all mailboxes type initially to -1
  for( i = 0; i <= number_of_threads; i++)
  {
    mailboxes[i].type = -1;
  }

  // initializes the threads 
 for(i =0; i<number_of_threads+1; i++)
  {
    if(sem_init(&threads[i], 0, 1) < 0)
    {
      perror("sem_init\n");
      exit(1);
    }
  }

 // creates threads
 for(i=1; i<number_of_threads+1; i++)
  {
    if(pthread_create(&idthreads[i], NULL, consumer, (void *)i) != 0)
    {
      perror("pthread_create\n");
      exit(1);
    }
  }

  // call to producer to distribute work among threads
 producer(number_of_threads, fileSize, search_max);
 
 // wait for threads to be done
 for(i=1; i< number_of_threads+1; i++)
 {
     (void)pthread_join(idthreads[i], NULL);
 }
 // destroy semaphores after work is done
 for(i=1; i < number_of_threads+1; i++)
 {
    (void)sem_destroy(&threads[i]);
 }
 
}

/* Send the message to mailbox at index iTo as a message pMsg*/
void SendMsg(int iTo, struct msg *pMsg)
{
  bool message_sent = false;

   while(1)
   {
      sem_wait(&threads[iTo]);
      // If mailbox is empty, write message into it.  Otherwise
      // will have to wait until it is empty.
      if(mailboxes[iTo].type == -1)
     {
        message_sent = true;
        mailboxes[iTo].iFrom = pMsg->iFrom;
        mailboxes[iTo].type = pMsg->type;
        mailboxes[iTo].value1 = pMsg->value1;
        mailboxes[iTo].value2 = pMsg->value2;
        mailboxes[iTo].firstCharStringLen = pMsg->firstCharStringLen;
        mailboxes[iTo].lastCharStringLen = pMsg->lastCharStringLen;
     }

     sem_post(&threads[iTo]);
     if(message_sent)
     {
       break;
     }
   }
}

/* message received from mailbox with index iFrom and sent with
 * message pMsg */
void RecvMsg(int iFrom, struct msg *pMsg)
{
    bool got_message = false;
    while(1)
    { 
        sem_wait(&threads[iFrom]);
        
        // If there is a message in the mailbox, read it.  Otherwise,
        // release semaphore and cycle back here.
        if(mailboxes[iFrom].type != -1)
        {
           got_message = true;
           pMsg->iFrom = mailboxes[iFrom].iFrom;
           pMsg->type = mailboxes[iFrom].type;
           pMsg->value1 = mailboxes[iFrom].value1;
           pMsg->value2 = mailboxes[iFrom].value2;
           pMsg->firstCharStringLen = mailboxes[iFrom].firstCharStringLen;
           pMsg->lastCharStringLen = mailboxes[iFrom].lastCharStringLen;
           mailboxes[iFrom].type = -1;
        }

        sem_post(&threads[iFrom]);
        
        if(got_message)
        {
          break;
        }
    }

}

/* producer distributes the work among other threads and then calculates the total
 * strings of a given length as threads come in. It also determines the max string size 
 * in the file.
 * int number_of_threads - is the number of threads given by the user
 * int fileSize - is the file size of the given file 
 * int *search_max - calculates the max file
 */
void *producer(int number_of_threads, int fileSize, int *search_max)
{
  int i;
  int split_buf_count;// size of buffer for each thread to read
  int remainder = fileSize % number_of_threads; // check to see if size of file is not evenly divisible by number of threads
  struct msg message;
  struct msg temp_message;
  int split_buf_string_size = 0;

  split_buf_count = fileSize/ number_of_threads;
  message.iFrom = 0;
  message.type = RANGE;
 
  // send message to worker thread with values of where to start reading buffer
  for(i=1; i <= number_of_threads; i++)
  {
    message.value1 = split_buf_count * ( i - 1);

    if(i == number_of_threads)
    {
      message.value2 = remainder + (split_buf_count * i) -1;
    }
    else 
    {
      message.value2 = (split_buf_count * i)-1;
    }

    message.firstCharStringLen = 0;
   SendMsg(i, &message);
  }

  // receive statistics from threads and validate them
  for(i = 1; i <= number_of_threads; i++)
  {
    int rmsg_index;

    RecvMsg(0, &temp_message);

    // Copy message to right slot in message_all_done array
    rmsg_index = temp_message.iFrom;    
    message_all_done[rmsg_index].type = temp_message.type;
    message_all_done[rmsg_index].value1 = temp_message.value1;
    message_all_done[rmsg_index].value2 = temp_message.value2;
    message_all_done[rmsg_index].lastCharStringLen = temp_message.lastCharStringLen;
    message_all_done[rmsg_index].firstCharStringLen = temp_message.firstCharStringLen;
  

    if(message_all_done[rmsg_index].type != ALLDONE)
    {
      perror("not all messages received\n");
    }

    count_w_threads += message_all_done[rmsg_index].value1;
  }

  // validate claims about statistics for worker threads
  for(i = 1; i < number_of_threads; i++)
  { 
    // check to see if value2 of message from thread is bigger than current search max value
    if(message_all_done[i].value2 > *search_max)
    {
     *search_max = message_all_done[i].value2;
    }
    
    // if last character and first character are greater than 0 they are both printable 
    // characters and should combine them as one string
     if(message_all_done[i].lastCharStringLen > 0  && message_all_done[i+1].firstCharStringLen > 0)
     {
         int sum;
         sum = message_all_done[i+1].firstCharStringLen + message_all_done[i].lastCharStringLen;
         if (sum >= 4)
	        {
              count_w_threads -= 1;
	        }
         else
	        {
	          count_w_threads -=2;
	        }

          // if not the first thread and message all done last char string length
          // is equal to split buffer count then count split buf string size without 
          // last char
         if( (i > 1) && message_all_done[i].lastCharStringLen == split_buf_count)
         {
              split_buf_string_size += message_all_done[i+1].firstCharStringLen;
         }
         else
	        {
	           split_buf_string_size +=  message_all_done[i+1].firstCharStringLen + message_all_done[i].lastCharStringLen;
         }
          
     }

     if(   (message_all_done[i].lastCharStringLen > 0 && message_all_done[i+1].firstCharStringLen == 0 && message_all_done[i].lastCharStringLen < 4 ) ||
	   (message_all_done[i].lastCharStringLen == 0 && message_all_done[i+1].firstCharStringLen > 0 && message_all_done[i+1].firstCharStringLen < 4))
       {
	      count_w_threads -=1;
       }
        
        // if search max is less than split string buf then change the size of search max
        // to split buf string size value    
     if( *search_max < split_buf_string_size)
     {
          *search_max = split_buf_string_size;
     }
  }
    // check the max string size of the last thread for search max
    if( *search_max < message_all_done[number_of_threads].value2)
     {
          *search_max = message_all_done[number_of_threads].value2;
     }

}

/*  the thread that does the work to calculate the max thread of a given buffered 
 *  region*/
void * consumer(void *arg)
{
  int i;
  int index = (int)arg;
  struct msg message_range_received;
  struct msg message_send;

  // Initialize message to send to main thread.
  message_send.iFrom = index;
  message_send.type = ALLDONE;
  message_send.value1 = 0;
  message_send.value2 = 0;
  message_send.firstCharStringLen = 0;
  message_send.lastCharStringLen =0;

  // Wait for Range message.
  RecvMsg(index, &message_range_received);
  
  // Process Range message.
  if(message_range_received.type == RANGE)
  {
        message_send.value1 = strings_length_more_than_four_w_thread(
				message_range_received.value1, // Starting point in buffer.
        message_range_received.value2,   // Ending point in buffer
        &message_send.value2,            // Return size of largest string
        &message_send.firstCharStringLen, // Return size of 1st string if 1st char is printable
       &message_send.lastCharStringLen);  // Return size of last string if last char is printable 
        SendMsg(0, &message_send);
  }
  else
  {
        // Bad message received.
        return;
  }
}

