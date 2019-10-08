#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#define S  1
#define F  0

#define TIMES 10

/*
 * data structure
 */
typedef struct _proc {
    char            name[2];
    int             data;
    pthread_t       thread;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    long int        waits;
} PROC;

void _exec_and_wait(void* args) {
    static struct timespec time_to_wait = {0, 0};
    PROC *proc = (PROC*) args;

    while(proc->data < TIMES) {
        time_to_wait.tv_sec = time(NULL) + proc->waits;
        pthread_mutex_lock(&proc->lock);
        pthread_cond_timedwait(&proc->cond, &proc->lock, &time_to_wait);
        proc->data++;
        printf("increase proc[%s] data to [%d]\n", proc->name, proc->data);
        pthread_mutex_unlock(&proc->lock);
    }
}

int _init_proc(PROC *proc, void *(*routine) (void *)) {
    pthread_attr_t attr;
    //
    printf("try to init proc [%s]\n", proc->name);
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    //
    pthread_mutex_init(&proc->lock, NULL);
    pthread_cond_init(&proc->cond, NULL);
    //
    pthread_create(&proc->thread, &attr, routine, (void*) proc);
    pthread_attr_destroy(&attr);
    //
    return S;
}

/*
 *  main
 */
int main(int argc, char** argv)
{
    int  result_a = 0, result_b = 0;
    PROC proc_a, proc_b;

    strcpy(proc_a.name, "A\0");
    proc_a.data = 0;
    proc_a.waits = 2L;

    strcpy(proc_b.name, "B\0");
    proc_b.data = 0;
    proc_b.waits = 1L;

    if (!_init_proc(&proc_a, _exec_and_wait))
        printf("Fail to init proc [%s]\n", proc_a.name);
    /* */
    if (!_init_proc(&proc_b, _exec_and_wait))
        printf("Fail to init proc [%s]\n", proc_b.name);
    /* */
    pthread_join(proc_a.thread, (void*) &result_a);
    pthread_join(proc_b.thread, (void*) &result_b);

    return (EXIT_SUCCESS);
}