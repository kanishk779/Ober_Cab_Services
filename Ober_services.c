#include"Ober_services.h"
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
	// actually you don't need these arrays as they can be incorporated in the respective structures
	int * cab_status_array;
	int * customer_status_array;
} state;
typedef struct rider
{
	int identifier;
	struct rider * next;
	int wait_time;
	int ride_time;
	int cab_type_number;
	pthread_mutex_t lock;
} rider;
typedef struct payment_server
{
	int identifier;
	struct payment_server * next;
	int busy_state;
	pthread_mutex_t lock;
}payment_server;
typedef struct cab
{
	int identifier;
	struct cab * next;
	int current_cab_type;
	pthread_mutex_t lock;	
}cab;

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
state st;
int rider_count;
int cab_id,payment_server_id,rider_id;
pthread_mutex_t book_cab_lock;
pthread_cond_t cab_free = PTHREAD_COND_INITIALIZER; // customer will wait on this, and cab will wait on it
pthread_cond_t customer_free = PTHREAD_COND_INITIALIZER; // cab will wait on this, and customer will signal it
pthread_cond_t payment_server_free = PTHREAD_COND_INITIALIZER; // customer will wait on this for payment
pthread_mutex_t mutex;
pthread_mutex_t payment_mutex;
void * riders_routine(void * args)
{
	// creates random parameters
	rider *ri = (rider *)args;
	ri->wait_time = rand()%5;
	ri->ride_time = rand()%3;
	ri->cab_type_number = rand()%2;
	ri->wait_time += 30;
	ri->ride_time += 5;

	printf("The rider with id %d has started waiting\n",ri->identifier );
	// define the time to wait
	struct timespec timeToWait;
    timeToWait.tv_sec = ri->wait_time;
    int not_found = true;
    time_t start_time;
    time(&start_time);
    cat * found_cab;
    int found_index;
    while(not_found)
    {
    	time_t curr_time;
		time(&curr_time);
		double diff = difftime(curr_time,start_t);
		timeToWait.tv_sec -= (int)diff;
		pthread_mutex_lock(&mutex);
    	pthread_cond_timedwait(&cab_free,&mutex,timeToWait); // update this time
    	cab * temp = cab_head;
    	
    	for(int i=0;i<st.total_cabs;i++)
    	{
    		if(st.cab_status_array[i] == true)
    		{
    			// now a cab has been found , so check if it matches your choice
    			if(temp->current_cab_type == free)
    			{
    				// riders want a premier ride
    				not_found = false;
    				found_index = i;
    				found_cab = temp;
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
    				st.cab_status_array[i] = false;
    				temp->current_cab_type = on_ride_pool_full_state;
    				break;
    			}
    		}
    		temp = temp->next;
    	}
    	pthread_mutex_unlock(&mutex);
    }
    // now do your ride by sleeping for ride-time
    printf("The rider with id %d started with riding for %d seconds\n",ri->identifier,ri->ride_time );
    sleep(ri->ride_time);
    // now the ride is over leave the cab
    pthread_mutex_lock(&mutex);
    st.cab_status_array[found_index] = true; // so it becomes free now
    // found_cab->current_cab_type = free_state  // but this is wrong because the ride could have been two people sharing the same cab
    // update the status of the car
    if(found_cab->current_cab_type == on_ride_pool_full_state)
    	found_cab->current_cab_type = on_ride_pool_one_state;
    else if(found_cab->current_cab_type == on_ride_pool_one_state || found_cab->current_cab_type == premier_ride_state)
    	found_cab->current_cab_type = free_state;
    pthread_mutex_unlock(&mutex);

    // now give the payment to the payment server
    // here also you need to wait for a payment server to be free
    int not_found_payment = true;
    payment_server * found_payment;
    while(not_found_payment)
    {
    	pthread_mutex_lock(&payment_mutex);
    	pthread_cond_wait(&payment_server_free,&payment_mutex);

    	// now you are out of the wait so check if the payment server is actually free
    	payment_server * temp = st.payment_server_head;
    	for(int i=0;i<st.total_payment_servers;i++)
    	{
    		if(temp->busy_state == false)
    		{
    			found_payment = temp;
    			not_found_payment = false;
    			break;
    		}
    	}
    	pthread_mutex_unlock(&payment_mutex);
    }
    // now pay the fees
    int random_time_for_fees = 1 + rand()%5;
    sleep(random_time_for_fees);
    printf("The rider with id %d has paid fees to payment server with id %d \n",ri->identifier,found_payment->identifier );
    
}

void * cab_routine(void * args)
{
	// the driver will have to wait for the signal from one of the customer
	// once its get the signal than it will check the customer array if it is available
	// if it finds the customer than 
}
void * payment_servers_routine(void * args)
{
	// the server will have to wait for the signal from one of the customer
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
			rider_head = new_rider;
			temp = new_rider;
		}
		else
		{
			temp->next = new_rider;
			temp = temp->next;
		}
		pthread_create(&st.rider_threads[i],NULL,&riders_routine,NULL);
	}
	for(int i=0;i<st.total_riders;i++)
	{
		pthread_join(st.rider_threads[i],NULL);
	}
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
			cab_head = new_cab;
			temp = new_cab;
		}
		else
		{
			temp->next = new_cab;
			temp = temp->next;
		}
		pthread_create(&st.cab_threads[i],NULL,&cab_routine,NULL);
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
		new_server->busy_state = false;
		if(i == 0)
		{	
			payment_server_head = new_server;
			temp = new_server;
		}
		else
		{
			temp->next = new_server;
			temp = temp->next;
		}
		pthread_create(&st.payment_server_threads[i],NULL,&servers_routine,NULL);
	}
}
int main()
{
	
	int rider,cab,server;
	printf("how many riders, cabs, servers do you want(in that order)\n");
	scanf("%d %d %d" riders,cabs,servers);
	st.cab_status_array = (int *)malloc(cabs * sizeof(int));
	for(int i=0;i<cabs;i++)
	{
		st.cab_status_array[i] = false; // it tells that the cab is not occupied
	}
	st.customer_status_array = (int *)malloc(riders * sizeof(int));
	for(int i=0;i<riders;i++)
	{
		st.customer_status_array[i] = false;
	}
	st.total_riders = riders;
	st.total_cabs = cabs;
	st.total_payment_servers = servers;

	// let the game begin

	// creating payment servers
}