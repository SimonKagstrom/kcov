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
	int i;

	// 10 seconds
	for (i = 0; i < 100; i++) {
		pthread_create(&thr, NULL, thread_cb, NULL);
		usleep(100 * 1000); // 100 ms
	}

	return NULL;
}


int main(int argc, const char *argv[])
{
	pthread_t tid;
	void *retv;

	pthread_create(&tid, NULL, real_thread_create, NULL);

	pthread_join(tid, &retv);

	return 0;
}

