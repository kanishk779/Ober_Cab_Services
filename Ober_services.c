#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<pthread.h>

typedef struct program_state
{
	int total_riders;
	int total_cabs;
	int total_payment_servers;
} state;

int main()
{
	state st;
	int rider,cab,server;
	printf("how many riders, cabs, servers do you want(in that order)\n");
	scanf("%d %d %d" riders,cabs,servers);
	st.total_riders = riders;
	st.total_cabs = cabs;
	st.total_payment_servers = servers;

	// let the game begin
}