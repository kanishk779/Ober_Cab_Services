#include"Ober_services.h"
// there is one thing that needs to implemented :-
// if the cab allocated to the user is pooled and that customer is the first one
// than don't decrease the number of the cabs in the system
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
    ri->wait_time *= 1000; // converting it into milli-seconds
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
    			// if the thread exceeded given amount of time than this thread must discontinue
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
    				if(ri->cab_type_number == 1) // make the cab busy only if it was a premier ride 
    					st.busy_cabs++;
    				st.cab_status_array[i] = false;
    				if(ri->cab_type_number == premier)
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
    int random_time_for_fees = 1 + rand()%5; // selecting random time to wait for giving fees
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
	// routine for cab
	return NULL;
}
void * payment_servers_routine(void * args)
{
	// this is the routine for payment server
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
		int random_sleep = 1 + rand()%2;
		sleep(random_sleep);
		pthread_create(&st.rider_threads[i],NULL,&riders_routine,(void *)new_rider);
	}
	// wait for all the rider threads to join
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
		// initialising a new cab
		cab * new_cab = (cab *)malloc(sizeof(cab));
		new_cab->identifier = ++cab_id;
		new_cab->current_cab_type = free_state;
		if(i == 0)
		{
			st.cab_head = new_cab; // setting the head of the linked list for cabs
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
		// initialising new payment server
		payment_server * new_server = (payment_server *)malloc(sizeof(payment_server));
		new_server->identifier = ++payment_server_id;
		new_server->busy_state = available;
		if(i == 0)
		{	
			st.payment_server_head = new_server; // setting the head of the linked list
			temp = new_server;
		}
		else
		{
			temp->next = new_server;
			temp = temp->next;
		}
		// creating the thread
		pthread_create(&st.payment_server_threads[i],NULL,&payment_servers_routine,(void *)new_server);
	}
}
int main()
{
	int riders,cabs,servers;
	printf("how many riders, cabs, servers do you want(in that order)\n");
	scanf("%d %d %d" ,&st.total_riders,&st.total_cabs,&st.total_payment_servers);
	
	st.busy_server = 0; // keeps track of number of busy servers
	st.busy_cabs = 0; // keeps track of number of busy cabs
	st.cab_status_array = (int *)malloc(st.total_cabs * sizeof(int));
	for(int i=0;i<cabs;i++)
	{
		st.cab_status_array[i] = available; // it tells that the cab is not occupied
	}
	// intialising the mutex required for cab_free , payment_server_free
	errno = pthread_mutex_init(&(mutex),NULL);
	if(errno)
		perror("mutex init error");
	errno = pthread_mutex_init(&payment_mutex,NULL);
	if(errno)
		perror("payment mutex error");
	create_cabs();
	create_payment_servers();
	create_riders();
}