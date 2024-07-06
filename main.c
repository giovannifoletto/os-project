#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

#define NUM_SEGMENTS 16
#define NUM_TRAINS 5

// we can set every percorso as a linked list with a queue on each node. This way, we can use the queue
// as the method to limit as one time the train (FIFO, the first train that arrive, the first that will go)
// and so to handle trains distributed state.

// maybe linked list not the greatest idea. Something more immediate would be better.

// Shared memory structure
typedef struct {
    char segments[NUM_SEGMENTS];
} SharedMemory;

// Function prototypes
void initialize_shared_memory(SharedMemory *shm);
void create_train_processes(SharedMemory *shm, const char *map_file);
void create_journey_process(const char *map_file);
void wait_for_children();

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <MAP1|MAP2>\n", argv[0]);
        exit(1);
    }

    // Create and attach shared memory
    int shmid = shmget(IPC_PRIVATE, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }
    
    SharedMemory *shm = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shm == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize shared memory
    initialize_shared_memory(shm);

    // Create JOURNEY process
    create_journey_process(argv[1]);

    // Create TRAIN_THR processes
    create_train_processes(shm, argv[1]);

    // Wait for all child processes to complete
    wait_for_children();

    // Clean up shared memory
    shmdt(shm);
    shmctl(shmid, IPC_RMID, NULL);

    printf("All trains have reached their destinations. Program complete.\n");

    return 0;
}

void initialize_shared_memory(SharedMemory *shm) {
    memset(shm->segments, '0', NUM_SEGMENTS);
}

void create_journey_process(const char *map_file) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: JOURNEY
        // TODO: Implement JOURNEY process logic
        exit(0);
    } else if (pid < 0) {
        perror("Fork failed for JOURNEY process");
        exit(1);
    }
}

void create_train_processes(SharedMemory *shm, const char *map_file) {
    for (int i = 0; i < NUM_TRAINS; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process: TRAIN_THR
            // TODO: Implement TRAIN_THR process logic
            exit(0);
        } else if (pid < 0) {
            perror("Fork failed for TRAIN_THR process");
            exit(1);
        }
    }
}

void wait_for_children() {
    int status;
    pid_t pid;
    while ((pid = wait(&status)) > 0) {
        if (WIFEXITED(status)) {
            printf("Child process %d exited with status %d\n", pid, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child process %d killed by signal %d\n", pid, WTERMSIG(status));
        }
    }
}