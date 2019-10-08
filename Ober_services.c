#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<pthread.h>
#include<time.h>

typedef struct program_state
{
	int total_riders;
	int total_cabs;
	int total_payment_servers;
	pthread_t * riders_threads;
	pthread_t * cabs_threads;
	pthread_t * payment_servers_threads;
} state;
struct rider_info
{
	int wait_time;
	int ride_time;
	int cab_type_number;
	int identifier;
};
state st;
enum cab_state
{
	free_state,
	premier_ride_state,
	on_ride_pool_full_state,
	on_ride_pool_one_state
}
enum cab_type
{
	premier,
	pool
}
int rider_count;
pthread_mutex_t book_cab_lock;
void book_cab(struct rider_info ri,time_t start_t)
{
	pthread_mutex_lock(&book_cab_lock);
	time_t curr_time;
	time(&curr_time);
	double diff = difftime(curr_time,start_t);
	if((int) diff > ri.wait_time)
	{
		printf("The rider with id %d exits without travelling\n",ri.identifier);
	}
	else
	{
		// try to find a free cab (maybe pool with one as well if the rider wishes)
		find_cab(ri); 

	}
	pthread_mutex_unlock(&book_cab_lock);
	return NULL;
}
void * riders_routine(void * args)
{
	// creates random parameters
	struct rider_info ri;
	ri.wait_time = rand()%5;
	ri.ride_time = rand()%3;
	ri.cab_type_number = rand()%2;
	ri.wait_time += 30;
	ri.ride_time += 5;
	
	rider_count++;
	ri.identifier = rider_count;
	// now a rider is alive in the system and will leave when he gets his chance
	// or may be he will have to leave without getting a ride
	printf("The rider with id %d has started waiting\n",rider_count );
	// start the time keeping 
	time_t start_t;
	time(&start_t);
	book_cab(ri,start_t);
}
void create_riders()
{
	riders_threads = (pthread_t *)malloc(st.total_riders * sizeof(pthread_t));
	for(int i=0;i<st.total_riders;i++)
	{
		pthread_create(&riders_threads[i],NULL,riders_routine,NULL);
	}
}
int main()
{
	
	int rider,cab,server;
	printf("how many riders, cabs, servers do you want(in that order)\n");
	scanf("%d %d %d" riders,cabs,servers);
	st.total_riders = riders;
	st.total_cabs = cabs;
	st.total_payment_servers = servers;

	// let the game begin

	// creating riders
}