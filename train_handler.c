#include "train_handler.h"
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void run(char *filename) {
  // setup memory and channels
  TrainSM tshm = create_share_memory();
  FileMapSM fmshm = create_shared_map();

  // Create JOURNEY process
  create_journey_process(filename, fmshm);

  // Create TRAIN_THR processes
  create_train_processes(tshm, fmshm);

  // finish and cleanup
  wait_for_all_trains();
  cleanup(tshm);

  printf("All trains have reached their destinations and memory freed. Program "
         "complete.\n");
}

TrainSM create_share_memory() {
  TrainSM ntshm;

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
  memset(shm->segments, '0', NUM_SEGMENTS);
  ntshm.shmid = shmid;
  ntshm.shm = shm;

  return ntshm;
}

void create_journey_process(char *filename, FileMapSM fmshm) {
  // the journey controller has its own process.
  pid_t pid = fork();
  if (pid == 0) {
    journey_process(filename, fmshm);
  } else if (pid < 0) {
    perror("Fork failed for Journey process");
    exit(1);
  }
}

FileMapSM create_shared_map() {
  FileMapSM fmshm;

  // Create and attach shared memory
  int shmid = shmget(IPC_PRIVATE, sizeof(TrainJourney)*NUM_TRAINS, IPC_CREAT | 0666);
  if (shmid == -1) {
    perror("shmget failed");
    exit(1);
  }

  TrainJourney *shm = (TrainJourney *)shmat(shmid, NULL, 0);
  if (shm == (void *)-1) {
    perror("shmat failed");
    exit(1);
  }

  fmshm.shmid = shmid;
  fmshm.tj = shm;

  return fmshm;
}

void journey_process(char *filename, FileMapSM fmshm) {
  printf("Journey Dispatcher created with Process ID: {%d}\n", getpid());

  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    perror("Error opening map file");
    exit(1);
  }

  char line[MAX_CHAR_LEN];
  TrainJourney *trains = malloc(sizeof(TrainJourney) * NUM_TRAINS);
  int train_idx = 0;

  while (fgets(line, sizeof(line), file)) {
    if (line[0] != 'T') { // check over file consistency
      perror("The file taken as input is not coherent with the specifications");
      exit(1);
    }

    // change end of line char
    char *current_pos = strchr(line, '\n');
    if (current_pos) {
      *current_pos = '\0';
    }

    trains[train_idx].itinerary = line;
  } // end getline
  printf("Finished importing map file\n");
  fclose(file);

  memcpy(fmshm.tj, trains, sizeof(TrainJourney)*NUM_TRAINS);
  perror("Create Journey memcpy");
  // wait for received messages
  // msg_type = 0 => send
  // msg_type = 1 => recv
  /*printf("Starting opening channels\n");
  int curr_sending = NUM_TRAINS - 1;
  while (curr_sending != 0) {
    Message req;
    Message rsp;
    // Wait until a request come's in
    msgrcv(msgid, &req, sizeof(req) - sizeof(long), curr_sending, 0);
    perror("Journey msgrcv");

    printf("Message received from train {%d}\n", req.train_id);

    // Setup response message
    rsp.train_id = req.train_id;
    rsp.msg_type = curr_sending;
    int tid = atoi(&req.train_id);

    strncpy(rsp.msg_text, trains[tid].itinerary, sizeof(rsp.msg_text));
    // perror("Journey strncpy");
    snprintf(rsp.msg_text, sizeof(rsp.msg_text), "%s", trains[tid].itinerary);
    // perror("Journey snprintf");
    msgsnd(msgid, &rsp, sizeof(rsp) - sizeof(long), req.msg_type);
    perror("Journey msgsnd");
    curr_sending--;
  }*/
  printf("Journey dispatcher exiting\n");
  exit(0);
}

void create_train_processes(TrainSM tshm, FileMapSM fmshm) {
  for (int i = 0; i < NUM_TRAINS; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      train_process(tshm.shm, fmshm, i);
    } else if (pid < 0) {
      perror("Fork failed for create_train_processes process");
      exit(1);
    }
  }
}

void train_process(SharedMemory *shm, FileMapSM fmshm, int train_num) {
  printf("Train Process id: {%d} - pid: {%d}\n", train_num, getpid());

  char* itinerary = malloc(sizeof(char)*MAX_CHAR_LEN);
  strcpy(itinerary, fmshm.tj[train_num].itinerary);
  printf("Itinerary received: {%s}", itinerary);
  /*char *itinerary = NULL;
  int is_empty = TRUE;
  while (is_empty) {
    Message req;
    Message rsp;

    req.msg_type = train_num; // Message type for requests
    req.train_id = train_num;
    itinerary = realloc(itinerary, sizeof(char) * sizeof(req.msg_text));
    strncpy(req.msg_text, itinerary, sizeof(req.msg_text));
    snprintf(req.msg_text, sizeof(req.msg_text), "%s", itinerary);
    msgsnd(msgid, &req, sizeof(req) - sizeof(long), train_num);
    perror("Train msgsnd");
    msgrcv(msgid, &rsp, sizeof(rsp) - sizeof(long), train_num, 0);
    perror("Train msgrcv");

    if (rsp.train_id != train_num) {
      continue;
    }

    if (strcmp(rsp.msg_text, "") != 0) {
      is_empty = FALSE;
    }

    printf("Next hop for train {%d} is: {%s}\n", train_num, rsp.msg_text);
  }*/
  exit(0);
}

// === UTILITIES ===
void log_journey(char *train_id, char *current, char *next) {
  FILE *log_file;
  char filename[10];
  sprintf(filename, "%s.log", train_id);
  log_file = fopen(filename, "a");
  if (log_file) {
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strcspn(timestamp, "\n")] = 0; // Remove newline character
    fprintf(log_file, "[Current: %s], [Next: %s], %s\n", current, next,
            timestamp);
    fclose(log_file);
  }
}

void wait_for_all_trains() {
  int status;
  pid_t pid;
  while ((pid = wait(&status)) > 0) {
    if (WIFEXITED(status)) {
      printf("Child process %d exited with status %d\n", pid,
             WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      printf("Child process %d killed by signal %d\n", pid, WTERMSIG(status));
    }
  }
}

void cleanup(TrainSM tshm) {
  shmdt(tshm.shm);
  perror("Clean shared memory returned");
  shmctl(tshm.shmid, IPC_RMID, NULL);
  perror("Clean shared memory ID returned");
}

int create_message_queue() {
  // create channels for message passing for the journey
  int msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
  if (msgid == -1) {
    perror("msgget failed");
    exit(1);
  }

  return msgid;
}

/*
char **import_whitespace_divided_journey() {

  // create a backup line copy to work with
  char *line_copy = strdup(line);
  if (!line_copy) {
    perror("Failed to allocate memory for line_copy string");
    exit(1);
  }
  // initialize the train_id field
  trains[train_idx].train_id = atoi(&line[1]);

  // splitting whole string over spaces
  char *token = strtok(line, " ");
  char **result = NULL;
  int size = 0;

  while (token != NULL) {
    char **temp = realloc(result, sizeof(char *) * (size + 1));
    if (!temp) {
      // failed to realloc
      free(result);
      free(line_copy);
      perror("Failed to realloc [create_journey_process]");
      exit(1);
    }
    result = temp;

    result[size] = strdup(token);
    if (!result[size]) {
      // failed to strdup
      for (int i = 0; i < size; i++) {
        free(result[i]);
      }
      free(result);
      free(line_copy);
      perror("Failed to strdup");
      exit(1);
    }
    size++;
    token = strtok(NULL, " ");
  } // end while splitting section
  free(line_copy);
  trains[train_idx].itinerary = result;
  trains[train_idx].num_segements = size;
}
*/
