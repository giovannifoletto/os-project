#include "train_handler.h"

pid_t create_process(void (*function)(void *), void *arg) {
  pid_t pid = fork();
  if (pid == 0) {
    function(arg);
    exit(0);
  }
  return pid;
}

void controller_process(void *arg) {
  SharedMemory *shared_memory = (SharedMemory *)arg;

  // Create JOURNEY process
  create_process(journey_process, NULL);

  // Create TRAIN_THR processes
  for (int i = 0; i < NUM_TRAINS; i++) {
    Train *train = malloc(sizeof(Train));
    train->train_id = i + 1;
    create_process(train_process, train);
  }

  // Wait for all trains to finish
  while (wait(NULL) > 0)
    ;

  printf("All trains have reached their destinations. Program complete.\n");
}

void train_process(void *arg) {
  Train *train = (Train *)arg;
  // TODO: Implement train logic
}

void journey_process(void *arg) {
  // TODO: Implement journey logic
}
