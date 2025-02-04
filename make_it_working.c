#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_SEGMENTS 16
#define NUM_TRAINS 5

// Shared track segments
char track_segments[NUM_SEGMENTS];

// Mutex for each track segment
pthread_mutex_t track_mutex[NUM_SEGMENTS];

// Train data structure
typedef struct {
  int train_id;
} Train;

// Simulates train journey
void *train_journey(void *arg) {
  Train *train = (Train *)arg;
  int train_id = train->train_id;

  printf("Train %d started its journey.\n", train_id);

  // Example journey (this should come from MAP1 or MAP2)
  int route[] = {0, 1, 2, 3, 8}; // Sample route for train
  int route_length = sizeof(route) / sizeof(route[0]);

  for (int i = 0; i < route_length; i++) {
    int segment = route[i];

    // Try to lock the track segment
    pthread_mutex_lock(&track_mutex[segment]);

    // Mark segment as occupied
    track_segments[segment] = '1';
    printf("Train %d entered segment %d.\n", train_id, segment);

    // Simulate travel time
    sleep(2);

    // Free the segment
    track_segments[segment] = '0';
    pthread_mutex_unlock(&track_mutex[segment]);
    printf("Train %d left segment %d.\n", train_id, segment);
  }

  printf("Train %d reached destination.\n", train_id);
  free(train);
  pthread_exit(NULL);
}

int main() {
  pthread_t trains[NUM_TRAINS];

  // Initialize track segments and mutexes
  memset(track_segments, '0', NUM_SEGMENTS);
  for (int i = 0; i < NUM_SEGMENTS; i++) {
    if (pthread_mutex_init(&track_mutex[i], NULL) == 0) {
      perror("Error while pthread_mutex_init: line 63\n");
      return 1;
    }
  }

  printf("Controller: Tracks initialized.\n");

  // Create train threads
  for (int i = 0; i < NUM_TRAINS; i++) {
    Train *train = malloc(sizeof(Train));
    train->train_id = i + 1;

    if (pthread_create(&trains[i], NULL, train_journey, train) != 0) {
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

  return 0;
}
