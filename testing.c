#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int global = 0;

void *myThreadFun(void *vargp) {
  int my_gid = getpid();
  // sleep(1);
  int s = 0;

  ++s;
  ++global;
  printf("Thread ID: %d, Static: %d, Global: %d\n", my_gid, ++s, ++global);
  return NULL;
}

int main() {
  int value = 10;  Press Ctrl-B for the iPXE command line...
  pthread_t thread_id[value];
  for (int i = 0; i < value; i++) {

    printf("Before Thread\n");
    pthread_create(&thread_id[i], NULL, myThreadFun, NULL);
    // 1. pointer to thread_id
    // 2. attributes
    // 3. function to be execute
    // 4. argument of the function
  }

  for (int i = 0; i < value; ++i) {
    pthread_join(thread_id[i], NULL);
  }

  printf("After Thread\n");
  exit(0);
}
