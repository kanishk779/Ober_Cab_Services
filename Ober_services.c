//#include"Ober_services.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<pthread.h>
#include<time.h>
#include<errno.h>
#include<sys/time.h>
#define true 1
#define false 0
#define available 1
#define busy 0
typedef struct rider
{
	int identifier;
	struct rider * next;
	int wait_time;
	int ride_time;
	int cab_type_number;
} rider;
typedef struct payment_server
{
	int identifier;
	struct payment_server * next;
	int busy_state;
}payment_server;
typedef struct cab
{
	int identifier;
	struct cab * next;
	int current_cab_type;	
}cab;
typedef struct program_state
{
	int total_riders;
	int total_cabs;
	int total_payment_servers;
	pthread_t * rider_threads;
	pthread_t * cab_threads;
	pthread_t * payment_server_threads;
	rider * rider_head;
	payment_server * payment_server_head;
	cab * cab_head;
	int busy_server;
	int busy_cabs;
	// actually you don't need these arrays as they can be incorporated in the respective structures
	int * cab_status_array;
	int * customer_status_array;
} state;


enum cab_state
{
	free_state,
	premier_ride_state,
	on_ride_pool_full_state,
	on_ride_pool_one_state
};
enum cab_type
{
	premier,
	pool
};
state st;
int rider_count;
int cab_id,payment_server_id,rider_id;
pthread_cond_t cab_free = PTHREAD_COND_INITIALIZER; // customer will wait on this, and cab will wait on it
pthread_cond_t customer_free = PTHREAD_COND_INITIALIZER; // cab will wait on this, and customer will signal it
pthread_cond_t payment_server_free = PTHREAD_COND_INITIALIZER; // customer will wait on this for payment
pthread_cond_t customer_free_for_payment = PTHREAD_COND_INITIALIZER; // customer will signal this when it busy
pthread_mutex_t mutex;
pthread_mutex_t payment_mutex;
void * riders_routine(void * args)
{
	// creates random parameters
	rider *ri = (rider *)args;
	ri->wait_time = rand()%5;
	ri->ride_time = rand()%4;
	ri->cab_type_number = rand()%2;
	ri->wait_time += 10;
	ri->ride_time += 5;

	printf("The rider with id %d has started waiting for cab of type %d wait time-%d ride time - %d\n",ri->identifier,ri->cab_type_number,ri->wait_time,ri->ride_time );
	// define the time to wait
	struct timeval tv;
    struct timespec ts;
    ri->wait_time *= 1000; // converting it into ms
    gettimeofday(&tv, NULL);
    ts.tv_sec = time(NULL) + ri->wait_time/1000;
    ts.tv_nsec = tv.tv_usec * 1000 + 1000 * 1000 * (ri->wait_time % 1000);
    ts.tv_sec += ts.tv_nsec / (1000 * 1000 * 1000);
    ts.tv_nsec %= (1000 * 1000 * 1000);
	
    int not_found = true;
    
    cab * found_cab;
    int found_index;
    while(not_found)
    {
		pthread_mutex_lock(&mutex);
		if(st.busy_cabs == st.total_cabs)
    	{
    		// since the ts specifies the absolute time we need not update wait time in each iteration
    		errno = pthread_cond_timedwait(&cab_free,&mutex,&ts); 
    		if(errno == ETIMEDOUT)
    		{
    			printf("The customer with id %d did not get any ride as wait time is finished\n",ri->identifier);
    			pthread_mutex_unlock(&mutex);
    			return NULL;
    		}
    	}
    	cab * temp = st.cab_head;
    	
    	for(int i=0;i<st.total_cabs;i++)
    	{
    		if(st.cab_status_array[i] == available)
    		{
    			// now a cab has been found , so check if it matches your choice
    			if(temp->current_cab_type == free_state)
    			{
    				// riders want a premier ride
    				not_found = false;
    				found_index = i;
    				found_cab = temp;
    				st.busy_cabs++;
    				st.cab_status_array[i] = false;
    				if(ri->cab_type_number == 0)
    					temp->current_cab_type = premier_ride_state; // updating the cab type
    				else
    					temp->current_cab_type = on_ride_pool_one_state; // updating the cab type
    				break;
    			}
    			else if((temp->current_cab_type == on_ride_pool_one_state) && (ri->cab_type_number == 1))
    			{
    				// rider want pool car
    				not_found = false;
    				found_index = i;
    				found_cab = temp;
    				st.busy_cabs++;
    				st.cab_status_array[i] = false;
    				temp->current_cab_type = on_ride_pool_full_state;
    				break;
    			}
    		}
    		temp = temp->next;
    	}
    	 // increase the numbers of busy cab
    	pthread_mutex_unlock(&mutex);
    }
    // now do your ride by sleeping for ride-time
    printf("The rider with id %d STARTED with riding for %d seconds in cab with id %d \n",ri->identifier,ri->ride_time,found_cab->identifier );
    sleep(ri->ride_time);
    printf("The rider with id %d FINISHED with riding for %d seconds in cab with id %d \n",ri->identifier,ri->ride_time,found_cab->identifier );
    // now the ride is over leave the cab
    pthread_mutex_lock(&mutex);
    st.cab_status_array[found_index] = available; // so it becomes free now
    // found_cab->current_cab_type = free_state  // but this is wrong because the ride could have been two people sharing the same cab
    // update the status of the car
    if(found_cab->current_cab_type == on_ride_pool_full_state)
    	found_cab->current_cab_type = on_ride_pool_one_state;
    else if(found_cab->current_cab_type == on_ride_pool_one_state || found_cab->current_cab_type == premier_ride_state)
    	found_cab->current_cab_type = free_state;
    st.busy_cabs--; // decrease the number of busy cabs
    // now signal that the car is free 
    pthread_cond_signal(&cab_free);
    pthread_mutex_unlock(&mutex);

    // now give the payment to the payment server
    // here also you need to wait for a payment server to be free
    int not_found_payment = true;
    payment_server * found_payment;
    while(not_found_payment)
    {
    	pthread_mutex_lock(&payment_mutex);
    	if(st.busy_server == st.total_payment_servers)
    	{
    		errno = pthread_cond_wait(&payment_server_free,&payment_mutex);
    		if(errno)
    			perror("payment server wait errno");
    	}

    	// now you are out of the wait so check if the payment server is actually free
    	payment_server * temp = st.payment_server_head;
    	for(int i=0;i<st.total_payment_servers;i++)
    	{
    		if(temp->busy_state == available)
    		{
    			found_payment = temp;
    			st.busy_server++;
    			temp->busy_state = busy;
    			not_found_payment = false;
    			break;
    		}
    		temp = temp->next;
    	}
    	pthread_mutex_unlock(&payment_mutex);
    }
    // now pay the fees
    int random_time_for_fees = 1 + rand()%5;
    printf("The rider with id %d is PAYING fees to payment server with id %d in time %d seconds\n",ri->identifier,found_payment->identifier,random_time_for_fees );
    sleep(random_time_for_fees);
    printf("The rider with id %d has PAID fees to payment server with id %d \n",ri->identifier,found_payment->identifier );
    pthread_mutex_lock(&payment_mutex);
    found_payment->busy_state = available;
    st.busy_server--;
    pthread_cond_signal(&payment_server_free);
    pthread_mutex_unlock(&payment_mutex);
    return NULL;
}

void * cab_routine(void * args)
{
	// the driver will have to wait for the signal from one of the customer
	// once its get the signal than it will check the customer array if it is available
	// if it finds the customer than
	cab * curr_cab = (cab *)args;
	//printf("The cab with id %d is active now\n", curr_cab->identifier);
	return NULL;
}
void * payment_servers_routine(void * args)
{
	// the server will have to wait for the signal from one of the customer
	payment_server * curr_server = (payment_server * )args;
	//printf("The server with id %d is active now\n",curr_server->identifier );
	return NULL;
}
void create_riders()
{
	st.rider_threads = (pthread_t *)malloc(st.total_riders * sizeof(pthread_t));
	rider * temp;
	for(int i=0;i<st.total_riders;i++)
	{
		rider * new_rider = (rider *)malloc(sizeof(rider));
		new_rider->identifier = ++rider_id;
		if(i == 0)
		{
			st.rider_head = new_rider;
			temp = new_rider;
		}
		else
		{
			temp->next = new_rider;
			temp = temp->next;
		}
		pthread_create(&st.rider_threads[i],NULL,&riders_routine,(void *)new_rider);
	}
	for(int i=0;i<st.total_riders;i++)
	{
		pthread_join(st.rider_threads[i],NULL);
	}
	printf("ALL riders completed their rides\n");
}
void create_cabs()
{
	st.cab_threads = (pthread_t *)malloc(st.total_cabs * sizeof(pthread_t));
	cab * temp;
	for(int i=0;i<st.total_cabs ;i++)
	{
		cab * new_cab = (cab *)malloc(sizeof(cab));
		new_cab->identifier = ++cab_id;
		new_cab->current_cab_type = free_state;
		if(i == 0)
		{
			st.cab_head = new_cab;
			temp = new_cab;
		}
		else
		{
			temp->next = new_cab;
			temp = temp->next;
		}
		pthread_create(&st.cab_threads[i],NULL,&cab_routine,(void *)new_cab);
	}
}
void create_payment_servers()
{
	st.payment_server_threads = (pthread_t *)malloc(st.total_payment_servers * sizeof(pthread_t));
	payment_server * temp;
	for(int i=0; i<st.total_payment_servers ;i++)
	{
		payment_server * new_server = (payment_server *)malloc(sizeof(payment_server));
		new_server->identifier = ++payment_server_id;
		new_server->busy_state = available;
		if(i == 0)
		{	
			st.payment_server_head = new_server;
			temp = new_server;
		}
		else
		{
			temp->next = new_server;
			temp = temp->next;
		}
		pthread_create(&st.payment_server_threads[i],NULL,&payment_servers_routine,(void *)new_server);
	}
}
int main()
{
	
	int riders,cabs,servers;
	printf("how many riders, cabs, servers do you want(in that order)\n");
	scanf("%d %d %d" ,&riders,&cabs,&servers);
	st.cab_status_array = (int *)malloc(cabs * sizeof(int));
	st.busy_server = 0;
	st.busy_cabs = 0;
	for(int i=0;i<cabs;i++)
	{
		st.cab_status_array[i] = available; // it tells that the cab is not occupied
	}
	st.customer_status_array = (int *)malloc(riders * sizeof(int));
	for(int i=0;i<riders;i++)
	{
		st.customer_status_array[i] = available;
	}
	st.total_riders = riders;
	st.total_cabs = cabs;
	st.total_payment_servers = servers;
	errno = pthread_mutex_init(&(mutex),NULL);
	if(errno)
		perror("mutex init error");
	errno = pthread_mutex_init(&payment_mutex,NULL);
	if(errno)
		perror("payment mutex error");
	// let the game begin

	// creating payment servers
	create_cabs();
	create_payment_servers();
	create_riders();
}