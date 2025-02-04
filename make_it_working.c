#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define NUM_SEGMENTS 16
#define NUM_TRAINS 5

// Shared memory key
#define SHM_KEY 1234

int main() {
    int shmid;
    char *track_segments;

    // Create shared memory for track segments
    shmid = shmget(SHM_KEY, NUM_SEGMENTS * sizeof(char), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    // Attach shared memory
    track_segments = (char *)shmat(shmid, NULL, 0);
    if (track_segments == (char *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize all track segments to '0' (free)
    memset(track_segments, '0', NUM_SEGMENTS);

    printf("Controller: Shared memory initialized.\n");

    // Create train processes
    pid_t train_pids[NUM_TRAINS];
    for (int i = 0; i < NUM_TRAINS; i++) {
        if ((train_pids[i] = fork()) == 0) {
            // Train process: Execute train program
            char train_id[3];
            sprintf(train_id, "%d", i + 1);
            execl("/usr/bin/echo", "train_process", train_id, NULL);
            perror("execl failed");
            exit(1);
        }
    }

    // Wait for all train processes to finish
    for (int i = 0; i < NUM_TRAINS; i++) {
        waitpid(train_pids[i], NULL, 0);
    }

    printf("Controller: All trains have reached their destination. Cleaning up.\n");

    // Detach and remove shared memory
    shmdt(track_segments);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
