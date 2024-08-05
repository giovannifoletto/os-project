#include "train_handler.h"
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

  // initialize Controller
  // =====================
  controller_start(shm, filename, shmid);
}

void controller_start(SharedMemory *shm, char *filename, int shmid) {

  // Initialize shared memory
  memset(shm->segments, '0', NUM_SEGMENTS);
  // create channels for message passing for the journey
  int msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
  if (msgid == -1) {
    perror("msgget for Journey failed");
    exit(1);
  }

  // Create JOURNEY process
  create_journey_process(filename, msgid);

  // Create TRAIN_THR processes
  create_train_processes(shm, msgid);

  // Wait for all child processes to complete
  wait_for_all_trains();

  // Clean up shared memory
  shmdt(shm);
  shmctl(shmid, IPC_RMID, NULL);

  printf("All trains have reached their destinations. Program complete.\n");
}

void create_journey_process(char *filename, int msgid) {
  // the journey controller has its own process.
  pid_t pid = fork();
  if (pid < 0) {
    perror("Fork failed for Journey process");
    exit(1);
  }

  printf("Entering Journey dispatcher");

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
  } // end getline
  fprintf(stdout, "Finished importing map file");
  fclose(file);

  // wait for received messages
  // msg_type = 0 => send
  // msg_type = 1 => recv
  while (TRUE) {
    Message req;
    Message rsp;

    // Wait until a request come's in
    msgrcv(msgid, &req, sizeof(req) - sizeof(long), 0, 0);
    printf("Message received from train {%d}\n", req.train_id);

    // if it is not for us, we skip
    if (req.msg_type == 0) {
      continue;
    }

    if (req.msg_action == Exit) {
      break;
    }
    if (req.msg_action == GetNext) {
      // train number
      int tn = req.train_id;
      // train current position
      char *tcp = malloc(sizeof(char) * MAX_MESSAGE_TEXT);
      strcpy(tcp, req.msg_text);
      int iter = 0;
      int not_current = TRUE;
      // find the current
      while (not_current && trains[tn].num_segements < iter) {
        if (strcmp(trains[tn].itinerary[iter], tcp)) {
          not_current = FALSE;
        }
        iter++;
      }
      // get the next

      char *next_hop = trains[tn].itinerary[iter + 1];

      // Setup response message
      rsp.msg_action = IsResponse;
      rsp.train_id = tn;
      rsp.msg_type = 1;
      strcpy(rsp.msg_text, next_hop);

      strncpy(rsp.msg_text, next_hop, sizeof(rsp.msg_text));
      snprintf(rsp.msg_text, sizeof(rsp.msg_text), "%s", next_hop);
      msgsnd(msgid, &rsp, sizeof(rsp) - sizeof(long), 0);
    }
  }

  printf("Journey dispatcher exiting");
  exit(0);
}

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

void create_train_processes(SharedMemory *shm, int msgid) {
  for (int i = 0; i < NUM_TRAINS; i++) {
    pid_t pid = fork();
    if (pid < 0) {
      perror("Fork failed for TRAIN_THR process");
      exit(1);
    }
    printf("Process id: {%d}\n", pid);
    int what_train_i_am = i;
    char *current_segment = "";
    char next_segment[10] = "NEXT";

    while (TRUE) {
      Message req;
      Message rsp;
      // if a station is returned, then I am arrived
      if (current_segment[0] == 'S') {
        break;
      }

      // request next_hop
      req.msg_type = 1; // Message type for requests
      req.train_id = what_train_i_am;
      strncpy(req.msg_text, next_segment, sizeof(req.msg_text));
      snprintf(req.msg_text, sizeof(req.msg_text), "%s", next_segment);
      msgsnd(msgid, &req, sizeof(req) - sizeof(long), 0);

      msgrcv(msgid, &rsp, sizeof(rsp) - sizeof(long), 0, 0);
      if (rsp.train_id != what_train_i_am) {
        continue;
      }
      printf("Next hop for train {%d} is: {%s}", what_train_i_am, rsp.msg_text);
    }
    exit(0);
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
