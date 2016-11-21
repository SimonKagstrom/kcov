#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>

void *thread_cb(void *ptr)
{
	printf("Thread Created\r\n");

	return NULL;
}

void* real_thread_create()
{
	pthread_t thr;

	while(1) {
		pthread_create(&thr, NULL, thread_cb, NULL);
		sleep(1);
	}
}


int main(int argc, const char *argv[])
{
	sem_t sem;
	pthread_t tid;

	pthread_create(&tid, NULL, real_thread_create, NULL);

	sem_init(&sem, 0, 0);
	sem_wait(&sem);

	exit(0);

	return 0;
}

