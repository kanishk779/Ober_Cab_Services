#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>

pthread_t tid[2];
int counter = 0;
pthread_mutex_t lock;
void * example(void * arg)
{
	pthread_mutex_lock(&lock);
	counter++;
	printf("the job number %d is started with id %u \n",counter,pthread_self() );
	sleep(3);

	printf("the job number %d is finished with id %u \n",counter,pthread_self() );
	pthread_mutex_unlock(&lock);
	return NULL;
}

int main()
{
	int i = 0;
	if(pthread_mutex_init(&lock,NULL) != 0)
	{
		printf("problem in creating lock\n");
		return 1;
	}
	while(i < 2)
	{
		int error = pthread_create(&tid[i],NULL,&example,NULL);
		if(error != 0)
			printf("problem in thread creation\n");
		i++;
	}

	pthread_join(tid[0],NULL);
	pthread_join(tid[1],NULL);
	pthread_mutex_destroy(&lock);
	return 0;
}