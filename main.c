#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define NUM_SEGMENTS 16
#define NUM_TRAINS 5
#define MAX_ROUTE 10

#define MSG_KEY 5678 // Message queue key
#define SHM_NAME "/my_shared_memory"
#define SHM_SIZE sizeof(int) * NUM_SEGMENTS

sem_t *track_semaphores[NUM_SEGMENTS];

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
  int start;
  int dest;
};

void load_routes(Train *, char *);
void journey_process(Train *);
int init_shared_memory();
void train_process(int);

// Function to initialize shared memory
int init_shared_memory() {
  int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) {
    perror("shm_open");
    exit(EXIT_FAILURE);
  }

  if (ftruncate(shm_fd, SHM_SIZE) == -1) {
    perror("init_shared_memory/ftruncate");
    exit(EXIT_FAILURE);
  }

  // this function will map the object in the process address space
  int *ptr = (int *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                         shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("init_shared_memory/mmap");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < NUM_SEGMENTS; i++) {
    ptr[i] = 0;
  }

  if (munmap(ptr, SHM_SIZE) == -1) {
    perror("init_shared_memory/munmap");
    exit(EXIT_FAILURE);
  }

  return shm_fd;
}

// Function to initialize POSIX semaphores
void init_semaphores() {
  for (int i = 0; i < NUM_SEGMENTS; i++) {
    char sem_name[20];
    snprintf(sem_name, sizeof(sem_name), "/track_sem_%d", i);
    sem_unlink(sem_name); // Ensure no previous instance exists
    // sem_open(sem_name, mode of the sem, permission, starting value);
    track_semaphores[i] = sem_open(sem_name, O_CREAT, 0666, 1);
    if (track_semaphores[i] == SEM_FAILED) {
      perror("init_semaphore/sem_open");
      exit(EXIT_FAILURE);
    }
  }
}

// Function for JOURNEY process to handle itineraries
void journey_process(Train *train_data) {
  printf("JOURNEY process started. Handling itineraries...\n");
  // Train train_data[NUM_TRAINS];
  // load_routes(train_data);

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
    message.start = train_data[i].start;
    message.dest = train_data[i].dest;
    message.num_steps = train_data[i].route_length;

    if (msgsnd(msgid, &message, sizeof(message) - sizeof(long), 0) == -1) {
      perror("msgsnd failed");
      exit(EXIT_FAILURE);
    }
  }
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
  printf("[DEBUG] Train %d received itinerary: S%d ", train_id, message.start);
  for (int i = 0; i < message.num_steps; i++) {
    printf("MA%d ", message.itinerary[i]);
  }
  printf("S%d\n", message.dest);

  // get shared pointer
  int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) {
    perror("shm_open");
    exit(EXIT_FAILURE);
  }
  // this function will map the object in the process address space
  int *ptr = (int *)mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
                         shm_fd, 0);
  if (ptr == MAP_FAILED) {
    perror("init_shared_memory/mmap");
    exit(EXIT_FAILURE);
  }

  // Open log file
  char log_filename[20];
  snprintf(log_filename, sizeof(log_filename), "T%d.log", train_id);
  FILE *log = fopen(log_filename, "w");
  if (log == NULL) {
    perror("Failed opening file");
    exit(EXIT_FAILURE);
  }

  int current_segment = -1;
  for (int i = 0; i < message.num_steps; ++i) {
    int next_segment = message.itinerary[i] - 1;
    sem_wait(track_semaphores[next_segment]);
    ptr[next_segment] = 1;
    // wait blocking call to block the semaphore
    if (current_segment != -1) {
      // sem_post unlock the semaphore
      ptr[current_segment] = 0;
      sem_post(track_semaphores[current_segment]);
    }
    // Log entry
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log,
            "[Current: %s%d], [Next: MA%d], %04d-%02d-%02d %02d:%02d:%02d\n",
            (current_segment == -1) ? "S" : "MA",
            (current_segment == -1) ? message.start : current_segment + 1,
            next_segment + 1, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);

    current_segment = next_segment;
    // printf("[DEBUG] Train %d moving to segment MA%d\n", train_id,
    //        message.itinerary[i]);
    sleep(2); // Simulating time taken to move
  }

  // unlock the last segment
  sem_post(track_semaphores[current_segment]);

  // Log entry
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  fprintf(log, "[Current: MA%d], [Next: S%d], %04d-%02d-%02d %02d:%02d:%02d\n",
          current_segment + 1, message.dest, t->tm_year + 1900, t->tm_mon + 1,
          t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

  fprintf(log, "[Current: S%d], [Next: --], %04d-%02d-%02d %02d:%02d:%02d\n",
          message.dest, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
          t->tm_hour, t->tm_min, t->tm_sec);

  printf("Train %d reached its destination.\n", train_id);

  if (munmap(ptr, SHM_SIZE) == -1) {
    perror("train_process/munmap");
    exit(EXIT_FAILURE);
  }

  if (close(shm_fd) == -1) {
    perror("train_process/close");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <MAP1|MAP2>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  Train train_data[NUM_TRAINS];
  char filename[20];
  snprintf(filename, sizeof(filename), "%s.txt", argv[1]);
  load_routes(train_data, filename);

  printf("Starting CONTROLLER process...\n");

  // Initialize shared memory and return
  // the shm filedescriptor
  int shm_fd = init_shared_memory();
  init_semaphores();

  // Fork JOURNEY process
  pid_t journey_pid = fork();
  if (journey_pid == 0) {
    journey_process(train_data);
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

  int statuses[NUM_TRAINS];
  for (int i = 0; i < NUM_TRAINS; i++) {
    statuses[i] = -1;
  }
  // Wait for all train processes to finish
  for (int i = 0; i < NUM_TRAINS; i++) {
    // why this wait(null) instead waitpid(train_pids[i])
    waitpid(train_pids[i], &statuses[i], 0);
    if (WIFEXITED(statuses[i])) {
      statuses[i] = WEXITSTATUS(statuses[i]);
    }
  }

  for (int i = 0; i < NUM_TRAINS; i++) {
    printf("[DEBUG] Process %d wiht pid %d exited with status: %d\n", i,
           train_pids[i], statuses[i]);
  }

  for (int i = 0; i < NUM_TRAINS; ++i) {
    // assert that all semaphores are set on zero after closing all.
  }

  if (close(shm_fd) == -1) {
    perror("close");
    return 1;
  }

  if (shm_unlink(SHM_NAME) == -1) {
    perror("train_process/shm_unlink");
    return 1;
  }
  // Cleanup shared memory on exit
  for (int i = 0; i < NUM_SEGMENTS; i++) {
    sem_close(track_semaphores[i]);
  }

  printf("Controller exiting, cleaned up shared memory.\n");
  return 0;
}

void load_routes(Train *trains, char *filename) {
  printf("[DEBUG] Using filename: '%s'\n", filename);

  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("Failed to open file");
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
