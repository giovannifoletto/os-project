// main.c (CONTROLLER)
#include "journey.h"
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

sem_t train_reading_map;

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <MAP1|MAP2>\n", argv[0]);
    exit(1);
  }

  // Create JOURNEY process
  // - create file reader shared memory
  int tj_shmid = shmget(IPC_PRIVATE, sizeof(TJSharedMemory), IPC_CREAT | 0666);
  TJSharedMemory *tjshm = (TJSharedMemory *)shmat(tj_shmid, NULL, 0);
  // blocked = TRUE
  printf("sem_init call RC: %d\n", sem_init(&train_reading_map, TRUE, TRUE));
  // get semaphore to write shared memory
  printf("sem_wait call RC: %d\n", sem_wait(&train_reading_map));
  train_journey_manager(argv[1], tjshm, tj_shmid);

  FILE *controller = fopen("tj_logger.txt", "w");
  if (controller == NULL) {
    perror("Error opening controller log file");
    exit(1);
  }

  printf("Waiting the journey map file to finish");
  printf("sem_wait 39 call RC: %d\n", sem_wait(&train_reading_map));
  printf("After waiting");
  for (int i = 0; i < 5; ++i) {
    print_train_journey(tjshm->tj[i], controller);
  }

  fclose(controller);
  printf("sem_destroy 46 call RC: %d\n", sem_destroy(&train_reading_map));
  // Create shared memory
  /*
int shmid = shmget(IPC_PRIVATE, sizeof(SharedMemory), IPC_CREAT | 0666);
  SharedMemory *shm = (SharedMemory *)shmat(shmid, NULL, 0);

  // Initialize shared memory
  memset(shm->segments, '0', NUM_SEGMENTS);

  // Create TRAIN_THR processes
  for (int i = 0; i < NUM_TRAINS; i++) {
    pid_t train_pid = fork();
    if (train_pid == 0) {
      // TRAIN_THR process code
      exit(0);
    }
  }

  // Wait for all child processes to finish
  for (int i = 0; i < NUM_TRAINS + 1; i++) {
    wait(NULL);
    }*/

  // Clean up file shared memory
  printf("shmdt call RC: %d\n", shmdt(tjshm));
  printf("shmctl call RC: %d\n", shmctl(tj_shmid, IPC_RMID, NULL));

  printf("All trains have reached their destinations. Simulation complete.\n");
  return 0;
}
