// #include "journey.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_SEGMENTS 16
#define NUM_TRAINS 5
#define MAX_ROUTE 10

// Shared track segments
char track_segments[NUM_SEGMENTS];

// Mutex for each track segment
pthread_mutex_t track_mutex[NUM_SEGMENTS];

// Barrier to synchronize train start
pthread_barrier_t train_barrier;

// Train data structure
typedef struct {
  int train_id;
  int route[MAX_ROUTE];
  int route_length;
  int start;
} Train;

// Load routes from MAP1.txt
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
        // && token[3] == S (this will go in error on normal cases)
        trains[train_index].start = atoi(&token[1]);
      }

      if (token[0] == 'M' && token[1] == 'A') {
        // Convert "MAx" to segment index
        trains[train_index].route[segment_index++] = atoi(token + 2);
      }
      token = strtok(NULL, " ");
    }

    trains[train_index].route_length = segment_index;
    train_index++;
    segment_index = 0;
  }

  fclose(file);
}

// Simulates train journey
void *train_journey(void *arg) {
  Train *train = (Train *)arg;
  int train_id = train->train_id;

  printf("Train %d prepared to be sync.\n", train_id);

  // Synchronize all trains before departure
  pthread_barrier_wait(&train_barrier);

  printf("Train %d started its journey.\n", train_id);

  for (int i = 0; i < train->route_length; i++) {
    int segment = train->route[i];

    // Try to lock the track segment
    pthread_mutex_lock(&track_mutex[segment]);
    // Mark segment as occupied
    track_segments[segment] = '1';
    printf("Train %d entered segment %d.\n", train_id, segment + 1);

    // Simulate travel time - uncomment if production
    sleep(2);

    // Free the segment
    track_segments[segment] = '0';
    pthread_mutex_unlock(&track_mutex[segment]);
    printf("Train %d left segment %d.\n", train_id, segment + 1);
  }

  printf("Train %d reached destination.\n", train_id);
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <MAP file>\n", argv[0]);
    return 1;
  }

  pthread_t trains[NUM_TRAINS];
  Train *train_data = (Train *)malloc((sizeof(Train) * NUM_TRAINS));

  // Initialize track segments and mutexes
  memset(track_segments, '0', NUM_SEGMENTS);
  for (int i = 0; i < NUM_SEGMENTS; i++) {
    if (pthread_mutex_init(&track_mutex[i], NULL) != 0) {
      perror("Error while pthread_mutex_init: line 63\n");
      return 1;
    }
  }

  if (pthread_barrier_init(&train_barrier, NULL, NUM_TRAINS) != 0) {
    perror("Error while pthread_mutex_init: line 63\n");
    return 1;
  }

  load_routes(train_data);

  printf("Controller: Tracks initialized.\n");

  // Create train threads
  for (int i = 0; i < NUM_TRAINS; i++) {
    if (pthread_create(&trains[i], NULL, train_journey, &train_data[i]) != 0) {
      perror("Failed to create train thread");
      return 1;
    }
  }

  // Wait for all train threads to finish
  for (int i = 0; i < NUM_TRAINS; i++) {
    pthread_join(trains[i], NULL);
  }

  printf("Controller: All trains reached destination. Cleaning up.\n");

  // Destroy mutexes
  for (int i = 0; i < NUM_SEGMENTS; i++) {
    pthread_mutex_destroy(&track_mutex[i]);
  }

  // Destroy barrier
  pthread_barrier_destroy(&train_barrier);

  free(train_data);

  return 0;
}
