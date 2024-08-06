#ifndef H_TRAIN_HANDLER
#define H_TRAIN_HANDLER

// Giovanni Foletto - Train Handler

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define NUM_SEGMENTS 16
#define NUM_TRAINS 5

typedef struct {
  char segments[NUM_SEGMENTS];
} SharedMemory;

typedef struct {
  char station_from;
  char station_to;
  char route[NUM_SEGMENTS];
  int route_length;
} Journey;

typedef struct {
  int train_id;
  Journey journey;
} Train;


void controller_process(void *arg);
void train_process(void *arg);
void journey_process(void *arg);


pid_t create_process(void (*function)(void *), void *arg);
#endif
