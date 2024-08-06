#include "train_handler.h"

int main(int argc, char *argv[]) {
    // This is the function controller
  if (argc != 2 ||
      (strcmp(argv[1], "MAP1") != 0 && strcmp(argv[1], "MAP2") != 0)) {
    fprintf(stderr, "Usage: %s <MAP1|MAP2>\n", argv[0]);
    return 1;
  }

  // Create shared memory
  int shm_id = shmget(IPC_PRIVATE, sizeof(SharedMemory), IPC_CREAT | 0666);
  if (shm_id == -1) {
    perror("shmget failed");
    return 1;
  }

  SharedMemory *shared_memory = (SharedMemory *)shmat(shm_id, NULL, 0);
  if (shared_memory == (void *)-1) {
    perror("shmat failed");
    return 1;
  }

  // Initialize shared memory
  memset(shared_memory->segments, '0', NUM_SEGMENTS);

  // Create CONTROLLER process
  create_process(controller_process, shared_memory);

  // Wait for all processes to finish
  while (wait(NULL) > 0)
    ;

  // Clean up shared memory
  shmdt(shared_memory);
  shmctl(shm_id, IPC_RMID, NULL);

  return 0;
}
