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