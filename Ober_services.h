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
	int wait_time;  // maximum wait time of the rider
	int ride_time;	// ride time of the rider
	int cab_type_number;	// type of car the rider wants
} rider;
typedef struct payment_server
{
	int identifier;
	struct payment_server * next;
	int busy_state;	// is the payment server busy
}payment_server;
typedef struct cab
{
	int identifier;
	struct cab * next;
	int current_cab_type;	// the type of service the cab is currently offering
}cab;
typedef struct program_state
{
	int total_riders;
	int total_cabs;
	int total_payment_servers;
	pthread_t * rider_threads;
	pthread_t * cab_threads;
	pthread_t * payment_server_threads;
	rider * rider_head;						// head of linked list of riders
	payment_server * payment_server_head;	// head of linked list of payment_servers
	cab * cab_head;							// head of linked list of cabs
	int busy_server;						// number of busy servers
	int busy_cabs;							// number of busy cabs
	int * cab_status_array;
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
int cab_id,payment_server_id,rider_id;
pthread_cond_t cab_free = PTHREAD_COND_INITIALIZER; // customer will wait on this, and cab will wait on it
pthread_cond_t customer_free = PTHREAD_COND_INITIALIZER; // cab will wait on this, and customer will signal it
pthread_cond_t payment_server_free = PTHREAD_COND_INITIALIZER; // customer will wait on this for payment
pthread_mutex_t mutex;
pthread_mutex_t payment_mutex;