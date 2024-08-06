#ifndef H_TRAIN_HANDLER
#define H_TRAIN_HANDLER

// Giovanni Foletto - OS's project - Train Handler libary

#define TRUE 1
#define FALSE 0

#define MAX_CHAR_LEN 256

#define NUM_SEGMENTS 16
#define NUM_TRAINS 5
#define MAX_STATIONS 8
#define MAX_ROUTE_LENGTH 10
#define MAX_MESSAGE_TEXT 100

#define MESSAGE_QUEUE 1234

// Shared memory structure
typedef struct {
  char segments[NUM_SEGMENTS];
} SharedMemory;

// helper struct to generate the SharedMemory
typedef struct TrainSM {
    SharedMemory *shm;
    int shmid;
} TrainSM;

// Train Journey struct
typedef struct TrainJourney {
  int train_id;
  char *itinerary;
} TrainJourney;

// helper struct to generate Journey SharedMemory
typedef struct FileMapSM {
    TrainJourney *tj;
    int shmid;
} FileMapSM;

// Structure for message passing
typedef struct {
  long msg_type;
  char train_id;
  char msg_text[MAX_MESSAGE_TEXT];
} Message;

// Function prototypes
// CONTROLLER
void run(char *filename);
TrainSM create_share_memory();
FileMapSM create_shared_map();
void wait_for_all_trains();
void cleanup(TrainSM tshm);
int create_message_queue();

// JOURNEY
void create_journey_process(char *filename, FileMapSM fmshm);
void journey_process(char *filename, FileMapSM fmshm);
void receive_journey();
void send_journey();

// TRAIN_THR
void create_train_processes(TrainSM tshm, FileMapSM fmshm);
void train_process(SharedMemory *shm, FileMapSM fmshm, int train_num);
/*
        route = request_route_from_journey()
        for segment in route:
            while True:
                if request_permission(segment):
                    update_shared_memory(segment, occupied)
                    log_movement(segment)
                    sleep(2)
                    update_shared_memory(previous_segment, free)
                    break
                else:
                    sleep(2)
        log_arrival_at_destination()
*/
#endif
