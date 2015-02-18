/* in mutexthr.c */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* gcc -o mutexthr mutexthr.c -lpthread */

void Deposit(int), BeginRegion(), EndRegion();
pthread_mutex_t mutex;  /* mutex id */
main()
{
    pthread_t idA, idB; /* ids of threads */
    void *MyThread(void *);

    if (pthread_mutex_init(&mutex, NULL) < 0) {
	perror("pthread_mutex_init");
	exit(1);
    }
    if (pthread_create(&idA, NULL, MyThread, (void *)"A") != 0) {
	perror("pthread_create");
	exit(1);
    }
    if (pthread_create(&idB, NULL, MyThread, (void *)"B") != 0) {
	perror("pthread_create");
	exit(1);
    }
    (void)pthread_join(idA, NULL);
    (void)pthread_join(idB, NULL);
    (void)pthread_mutex_destroy(&mutex);
}	

int balance = 0;  /* global shared variable */

void *MyThread(void *arg)
{
    char *sbName;

    sbName = (char *)arg;
    Deposit(10);
    printf("Balance = %d in Thread %s\n", balance, sbName);
}    

void Deposit(int deposit)
{
    int newbalance; /* local variable */

    BeginRegion();   /* enter critical region */
    newbalance = balance + deposit;
    balance = newbalance;
    EndRegion();   /* exit critical region */
}    

void BeginRegion()
{
    pthread_mutex_lock(&mutex);
}

void EndRegion()
{
    pthread_mutex_unlock(&mutex);
}
