# Ober_Cab_Services
## Synchronization Problem
* This project is done as part of the Operating System course which i took in my university.
* The problem is about designing a Uber like car service.
* The component of this problem will be **Riders , Cabs , Payment Servers**.
* The Riders will ask the Cabs to pick them up and drop them at their destination.
* The Cabs on getting this signal will fulfil the request of the rider.
* The Rider can request for a pool-cab or a premier ride.
* If the rider does not get the Cab within the limit of the wait-time, than it has to leave the system (without riding).
* In case of the premier the cab which services the rider has to be free.
* Once the ride is finished the rider gets off the cab and proceeds to pay the fees.
* This is done by using one of the free payment servers.

## My Implementation
* I have used two condition variable and two mutexes.
* Condition variable cab_free is used by riders who have not got their ride to wait on.
* cab_free is signalled by the rider which finishes it ride.
* Condition variable payment_server_free is used by riders who have not paid the fees to wait on.
* payment_server_free is signalled by the riders who have paid their fees.
* Mutexes are used to make the data manipulation atomic.
* The program starts by creating threads for each Cab, Rider, Payment servers.
* The Rider initiates the process by finding a car which is available and mathces it requirements.
* The order of riders in the system is random and this is achieved by making the thread sleep for some random amount of time before calling the method.
* The rider generate it ride-time , wait-time , cab-type for service randomly.
* If all the cabs are busy than the riders have to wait for till one of them becomes available.
* Once the riders starts it journey it marks that cab as un-available.
* The rider than will ride for the ride-time it generated randomly.
* When the ride is over the rider prints the appropriate message and also set the cab free, so the other rider can use it.
* It does so by signal on the condition variable.
* Now the riders wait for some payment server to become free.
* When they find a free payment server they engage in the process of paying the fees.
* The time to pay the fees is randomly allocated.
* When the server becomes free it is signalled by the rider.
* After this the rider successfully goes out of the system. 
