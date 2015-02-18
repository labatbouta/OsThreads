/* addem.c */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>

#define RANGE 1
#define ALLDONE 2
#define MAXTHREAD 10

/* message structure */
struct msg{
  int iFrom;
  int type;
  int value1;
  int value2;
};

/* global variables */
struct msg mailboxes[MAXTHREAD+1];
sem_t semaphore_threads[MAXTHREAD+1];
pthread_t pthreads[MAXTHREAD+1];

/* prototype of functions */
void *producer(int value1, int value2);
void *consumer(void *arg);
void SendMsg(int iTo, struct msg *pMsg);
void RecvMsg(int iFrom, struct msg *pMsg);
 
int main(int argc, char **argv){
    /* argc -- number of arguments */
    /* argv -- an array of strings */
    /* numOfThreads -- number of threads user wants*/
    /* sum-- number to sum up to from 1*/
    /* t-- is the index for for loops*/
    int numOfThreads;
    int sum;
    int t; 

 if(argc == 3){
    numOfThreads = atoi(argv[1]);
    sum = atoi(argv[2]);
    if(numOfThreads >  MAXTHREAD){
      perror("Cannot be more than 10 threads\n");
      exit(1);
    }
    if(numOfThreads <= 0){
      perror("number of threads must be greater than 0\n"); 
      exit(1);
    }
 }
 /*initialize mailbox type as -1 for no input*/
 for(t=0; t<=numOfThreads; t++){
  mailboxes[t].type = -1;
  }

 /* initialize semaphores */
 for(t=0; t<= numOfThreads; t++){
  if(sem_init(&semaphore_threads[t], 0, 1) < 0){
    perror("sem_init");
    exit(1);
    } 
 }


/* create threads */
for(t=1; t <= numOfThreads; t++){
  if( pthread_create(&pthreads[t], NULL, consumer, (void *)t) !=0){
    perror("pthread_create");
    exit(1);
  }
}

producer(numOfThreads, sum);

/*pthread_join*/
 for(t=1; t<= numOfThreads; t++){
   (void) pthread_join(pthreads[t], NULL);
   }

/* destroy semaphores after done*/
  for(t=1; t<= numOfThreads; t++){
  (void) sem_destroy(&semaphore_threads[t]);
  }
  
}        

/* Send the message to mailbox at index iTo as a message pMsg*/
void SendMsg(int iTo, struct msg *pMsg){
bool message_sent = false;

  while(1){
  sem_wait(&semaphore_threads[iTo]);
  if(mailboxes[iTo].type == -1){
     message_sent = true;
     mailboxes[iTo].iFrom = pMsg->iFrom;
     mailboxes[iTo].type = pMsg->type;
     mailboxes[iTo].value1 = pMsg->value1;
     mailboxes[iTo].value2 = pMsg->value2;
  }


sem_post(&semaphore_threads[iTo]);
  if(message_sent){
    break;
  }
}
}

/* message received from mailbox with index iFrom and sent with
 * message pMsg*/
void RecvMsg(int iFrom, struct msg *pMsg){
bool got_message = false;

while(1){  
  sem_wait(&semaphore_threads[iFrom]);

  if(mailboxes[iFrom].type != -1){
     got_message = true;
     pMsg->iFrom = mailboxes[iFrom].iFrom;
     pMsg->type = mailboxes[iFrom].type;
     pMsg->value1 = mailboxes[iFrom].value1;
     pMsg->value2 = mailboxes[iFrom].value2;
     mailboxes[iFrom].type = -1;
   }

  sem_post(&semaphore_threads[iFrom]);
  if(got_message){
    break;
  }

  }

}


/* producer sends the message to the consumer and receives the message
 * from the consumer of the total*/
void *producer(int numOfThreads, int countValue)
{
    int i;
    struct msg message;/*sent*/
    struct msg message_received;
    message.iFrom= 0;
    message.type = RANGE; 
    message.value1 = 1; 
    message.value2 = countValue;
    
    for (i=1; i<=numOfThreads; i++) {
       
        SendMsg(i , &message);
      
    }
    
    for( i=0 ;i<numOfThreads; i++){

        RecvMsg(0, &message_received);
        if(message_received.type != ALLDONE){
        perror("message is not done\n");    
        }
      }

     printf("The total for 1 to %d using %d threads is %d.\n", countValue, numOfThreads, message_received.value2);
     fflush(stdout);
     //  pthread_exit();
   
}       

/* consumer receives a message from the parent and sends 
 * back the total sum of values */
void *consumer(void *arg)
{   
    int index = (int)arg;
    int i;
    int total = 0;
    struct msg message1;
    struct msg message2;

    RecvMsg(index, &message1);

    for(i=0; i< message1.value2; i++){
      total +=mailboxes[index].value1;
      mailboxes[index].value1 +=1;


    }
    message2.iFrom = message1.iFrom;
    message2.type = ALLDONE;
    message2.value1 = message1.value1;
    message2.value2 = total;

   SendMsg(0, &message2);
    
}
