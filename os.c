#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>

#define totalcustomers 50
#define totalPostalWorkers 3

/***************************************SEMAPHORES*************************************/
sem_t max_capacity;
sem_t customer_ready, postalWorker_ready, finished[totalcustomers], askOrder[totalcustomers], placeOrder[totalPostalWorkers];
sem_t scales_ready, mutex1, mutex2, mutex3;

/*****************************************PIPES****************************************/
int orderNos[2];
int customerIDs[2];
int postalWorkerIDs[2];


/************************************ FUNCTION PROTOTYPES *******************************/
void enqueue(int,int);
int dequeue(int);
void initSemaphores();
void createQueues();



/************************************ Customer Thread Code ******************************/
void *customer(void* id)
{
    int customerID = *(int*)id;
    printf("Customer %d created\n",customerID);
	fflush(stdout);
	int orderNo, cPostalWorkerID;
	sem_wait(&max_capacity);
	printf("Customer %d enters post office\n",customerID);
	sem_wait(&postalWorker_ready);
	sem_wait(&mutex1);
	orderNo = rand()%3;;
	enqueue(0, customerID);
	sem_post(&customer_ready);
	sem_post(&mutex1);
	sem_wait(&askOrder[customerID]);
	sem_wait(&mutex2);
	cPostalWorkerID = dequeue(1);
	sem_post(&mutex2);
	printf("Customer %d asks postal worker %d to ", customerID, cPostalWorkerID);
	fflush(stdout);
	switch(orderNo)
	{
		case 0:
			printf("buy stamps\n");
			break;
		case 1:
			printf("mail a letter\n");
			break;
		case 2:
			printf("mail a package\n");
			break;
	}
	fflush(stdout);
	sem_wait(&mutex3);
	enqueue(2, orderNo);
	sem_post(&placeOrder[cPostalWorkerID]);
	sem_post(&mutex3);
	sem_wait(&finished[customerID]);
	switch(orderNo)
	{
		case 0:
			printf("Customer %d finished buying stamps\n", customerID);
			break;
		case 1:
			printf("Customer %d finished mailing letter\n", customerID);
			break;
		case 2:
			printf("Customer %d finished mailing package\n", customerID);
			break;
	}
	fflush(stdout);
	printf("Customer %d leaves post office\n",customerID);
	fflush(stdout);
	sem_post(&max_capacity);
	/* return arg back to the join function */
    pthread_exit(id);
}



/*********************************** POSTAL WORKER THREAD CODE****************************/
void *postalWorker(void* id)
{
	int postalWorkerID = *(int *)id;
    printf("Postal worker %d created\n",postalWorkerID);
	
	while(1)
	{
		fflush(stdout);
		int pwCustomerID,pwOrderNo;
		struct timespec req = {0};

		sem_post(&postalWorker_ready);
		sem_wait(&customer_ready);
		sem_wait(&mutex1);
		pwCustomerID = dequeue(0);
		sem_post(&mutex1);
		printf("Postal worker %d serving customer %d\n",postalWorkerID, pwCustomerID);
		fflush(stdout);
		sem_wait(&mutex2);
		enqueue(1, postalWorkerID);
		sem_post(&askOrder[pwCustomerID]);
		sem_post(&mutex2);
		sem_wait(&placeOrder[postalWorkerID]);
		sem_wait(&mutex3);
		pwOrderNo = dequeue(2);
		sem_post(&mutex3);
		switch(pwOrderNo)
		{
			case 0:
				req.tv_sec = 1;
				req.tv_nsec = 0L;
				nanosleep(&req, (struct timespec *)NULL);
				break;
			case 1:
				req.tv_sec = 1;
				req.tv_nsec = 500 * 1000000L;
				nanosleep(&req, (struct timespec *)NULL);
				break;
			case 2:
				sem_wait(&scales_ready);
				printf("Scales in use by postal worker %d\n",postalWorkerID);
				fflush(stdout);
				req.tv_sec = 2;
				req.tv_nsec = 0L;
				nanosleep(&req, (struct timespec *)NULL);
				printf("Scales released by postal worker %d\n",postalWorkerID);
				sem_post(&scales_ready);
				fflush(stdout);
				break;
			default:
				printf("Messed up Program");
		}
		printf("Postal worker %d finished serving Customer %d\n", postalWorkerID, pwCustomerID);
		fflush(stdout);
		sem_post(&finished[pwCustomerID]);
	}
}




/****************************** MAIN THREAD'S CODE *********************************/
int main(int argc,char *argv[])
{
    int worker;
    pthread_t customerThreads[totalcustomers], postalWorkerThreads[totalPostalWorkers];
    int customerNo, postalWorkerNo;
	int custids[totalcustomers];
	int pwids[totalPostalWorkers];
    int errcode;
    int *status;

	//Initialise the semaphores
	initSemaphores();
	createQueues();


    // create the Customer threads
	for (customerNo=0; customerNo<totalcustomers; customerNo++) 
    {
		custids[customerNo]=customerNo;
        // create thread
        if (errcode = pthread_create(&customerThreads[customerNo],NULL,customer,&custids[customerNo]))
        {
            printf("Thread could not be created");
        }
	}
	
	// create the Postal Worker threads
    for (postalWorkerNo=0; postalWorkerNo<totalPostalWorkers; postalWorkerNo++) 
    {
		pwids[postalWorkerNo]=postalWorkerNo;
        // create thread
        if (errcode = pthread_create(&postalWorkerThreads[postalWorkerNo],NULL,postalWorker,&pwids[postalWorkerNo]))
        {
            printf("Thread could not be created");
        }
    }

	//join the customer threads as they exit
    for (customerNo=0; customerNo<totalcustomers; customerNo++)
    {
        if(errcode=pthread_join(customerThreads[customerNo],(void*)&status))
        {
           printf("Thread could not be joined");
        }
        
      //check thread's exit status, should be the same as the thread number for this example
        if (*status != customerNo) 
        {
			printf("Customer thread %d terminated abnormally\n\n",customerNo);
			exit(1);
        }
		else
		{
			printf("Customer %d joined\n",*status);
		}

    }
	
    return(0);
}



void initSemaphores()
{
	int count;
	sem_init (&max_capacity, 0, 10);
	sem_init (&customer_ready, 0, 0);
	sem_init (&postalWorker_ready, 0, 0);
	for (count=0 ;count<totalcustomers ; count++)
	{
		sem_init (&finished[count], 0, 0);
		sem_init (&askOrder[count], 0, 0);
	}
	
	for (count=0 ;count<totalPostalWorkers ;count++ )
	{
		sem_init (&placeOrder[count], 0, 0);
	}

	sem_init (&scales_ready, 0, 1);
	sem_init (&mutex1, 0, 1);
	sem_init (&mutex2, 0, 1);
	sem_init (&mutex3, 0, 1);
}


void enqueue(int queue, int value)
{
	switch(queue)
	{
		case 0:
			write(customerIDs[1], &value, sizeof(int));
			break;
		case 1:
			write(postalWorkerIDs[1], &value, sizeof(int));
			break;
		case 2:
			write(orderNos[1], &value, sizeof(int));
			break;
	}
}

int dequeue(int queue)
{
	int value;
	switch(queue)
	{
		case 0:
			read(customerIDs[0], &value, sizeof(int));
			break;
		case 1:
			read(postalWorkerIDs[0], &value, sizeof(int));
			break;
		case 2:
			read(orderNos[0], &value, sizeof(int));
			break;
	}
	return value;
}

void createQueues()
{
	if (pipe(customerIDs) == -1) {
		printf("Pipe creation failed");
        exit(1);
    }

	if (pipe(postalWorkerIDs) == -1) {
		printf("Pipe creation failed");
		exit(1);
    }

	if (pipe(orderNos) == -1) {
		printf("Pipe creation failed");
		exit(1);
    }

}