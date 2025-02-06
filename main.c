#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_SEGMENTS 16
#define NUM_TRAINS 5
#define MAX_ROUTE 10

#define SHM_KEY 1234 // Shared memory key
#define MSG_KEY 5678 // Message queue key

// Train data structure
typedef struct {
  int train_id;
  int route[MAX_ROUTE];
  int route_length;
  int start;
  int dest;
} Train;

struct msg_buffer {
  long msg_type;
  int itinerary[NUM_SEGMENTS];
  int num_steps;
};

void load_routes(Train *);
void journey_process();
int *init_shared_memory();
void train_process(int);

// Function to initialize shared memory
int *init_shared_memory() {
  int shmid = shmget(SHM_KEY, NUM_SEGMENTS * sizeof(int), IPC_CREAT | 0666);
  if (shmid == -1) {
    perror("shmget failed");
    exit(1);
  }
  int *shm_ptr = (int *)shmat(shmid, NULL, 0);
  if (shm_ptr == (int *)-1) {
    perror("shmat failed");
    exit(1);
  }
  // Initialize track segments to 0 (free state)
  for (int i = 0; i < NUM_SEGMENTS; i++) {
    shm_ptr[i] = 0;
  }
  return shm_ptr;
}

// Function for JOURNEY process to handle itineraries
void journey_process() {
  printf("JOURNEY process started. Handling itineraries...\n");
  Train train_data[NUM_TRAINS];
  load_routes(train_data);

  for (int i = 0; i < NUM_TRAINS; ++i) {
    printf("Train %d: (id: %d, route_len: %d, start: %d, destination: %d)\n", i,
           train_data[i].train_id, train_data[i].route_length,
           train_data[i].start, train_data[i].dest);
  }

  int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
  if (msgid == -1) {
    perror("msgget failed");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < NUM_TRAINS; i++) {
    struct msg_buffer message;
    message.msg_type = train_data[i].train_id;
    memcpy(message.itinerary, train_data[i].route,
           sizeof(int) * train_data[i].route_length);
    message.num_steps = train_data[i].route_length;

    if (msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1) {
      perror("msgsnd failed");
      exit(1);
    }
  }

  // while (1) {
  //   sleep(5); // Simulating work
  // }
}

// Function for TRAIN_THR processes to move along the tracks
void train_process(int train_id) {
  printf("[DEBUG] Train %d started its journey with pid: %d\n", train_id,
         getpid());

  int msgid = msgget(MSG_KEY, 0666);
  if (msgid == -1) {
    perror("msgget failed in train process");
    exit(EXIT_FAILURE);
  }

  struct msg_buffer message;
  if (msgrcv(msgid, &message, sizeof(message) - sizeof(long), train_id, 0) ==
      -1) {
    perror("msgrecv failed");
    exit(EXIT_FAILURE);
  }

  // Debug: Print received itinerary
  printf("[DEBUG] Train %d received itinerary: ", train_id);
  for (int i = 0; i < message.num_steps; i++) {
    printf("MA%d ", message.itinerary[i]);
  }
  printf("\n");

  for (int i = 0; i < message.num_steps; i++) {
    printf("[DEBUG] Train %d moving to segment MA%d\n", train_id,
           message.itinerary[i]);
    sleep(2); // Simulating time taken to move
  }

  printf("[DEBUG] Train %d reached its destination.\n", train_id);
  exit(EXIT_SUCCESS);
}

int main() {
  printf("Starting CONTROLLER process...\n");

  // Initialize shared memory
  int *track_segments = init_shared_memory();

  // Fork JOURNEY process
  pid_t journey_pid = fork();
  if (journey_pid == 0) {
    journey_process();
    exit(0);
  }

  // Fork TRAIN_THR processes
  pid_t train_pids[NUM_TRAINS];
  for (int i = 0; i < NUM_TRAINS; i++) {
    train_pids[i] = fork();
    if (train_pids[i] == 0) {
      train_process(i + 1);
    }
  }

  // Wait for all train processes to finish
  for (int i = 0; i < NUM_TRAINS; i++) {
    // why this wait(null) instead waitpid(train_pids[i])
    wait(NULL);
  }

  // Cleanup shared memory on exit
  shmdt(track_segments);
  shmctl(shmget(SHM_KEY, NUM_SEGMENTS * sizeof(int), 0666), IPC_RMID, NULL);

  printf("Controller exiting, cleaned up shared memory.\n");
  return 0;
}

void load_routes(Train *trains) {
  FILE *file = fopen("MAP1.txt", "r");
  if (!file) {
    perror("Failed to open MAP1.txt");
    exit(1);
  }

  char line[200];
  int train_index = 0;

  while (fgets(line, sizeof(line), file) != 0 && train_index < NUM_TRAINS) {
    trains[train_index].train_id = train_index + 1;

    char *token = strtok(line, " ");
    int segment_index = 0;

    while (token && segment_index < MAX_ROUTE) {
      if (token[0] == 'S') {
        if (segment_index == 0) {
          trains[train_index].start = atoi(&token[1]);
        } else {
          trains[train_index].dest = atoi(&token[1]);
        }
      }

      if (token[0] == 'M' && token[1] == 'A') {
        // Convert "MAx" to segment index
        trains[train_index].route[segment_index++] = atoi(&token[2]);
      }
      token = strtok(NULL, " ");
    }

    trains[train_index].route_length = segment_index;
    train_index++;
    segment_index = 0;
  }

  fclose(file);
}
