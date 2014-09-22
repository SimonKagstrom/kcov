#include <iostream>
#include <fstream>
#include<pthread.h>
#include<sys/types.h>
#include<unistd.h>

void* thread_main(void *) {
  std::ofstream ofs("thread.log");
  int i = 0;
  while(true) {
    ofs << ++i << std::endl;
    sleep(1);
  }

  return NULL;
}

int main() {
  pthread_t t;
  int thread_code = pthread_create(&t, NULL, thread_main, NULL);

  if (thread_code < 0)
	  return 1;

  std::ofstream ofs("main.log");
  int i = 0;
  while(true) {
    ofs << ++i << std::endl;
    sleep(1);
  }
  return 0;
}
