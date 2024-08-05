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
#define MAX_MESSAGE_TEXT 10

// Shared memory structure
typedef struct {
  char segments[NUM_SEGMENTS];
} SharedMemory;

// Train Journey struct
typedef struct TrainJourney {
    int train_id;
    int num_segements;
    char** itinerary;
} TrainJourney;

// enum for actions
typedef enum MessageActions {
    GetNext,
    IsResponse,
    Exit,
} MessageActions;

// Structure for message passing
typedef struct {
    long msg_type;
    char train_id;
    MessageActions msg_action;
    // refactor to msg_text
    char msg_text[MAX_MESSAGE_TEXT];
} Message;

// Function prototypes
void run(char *filename);

// CONTROLLER
void controller_start(SharedMemory *shm, char *filename, int shmid);
void create_journey_process(char* filename, int msgid);
void create_train_processes(SharedMemory *shm, int msgid);
void wait_for_all_trains();
void cleanup();

// JOURNEY
void load_maps_from_file(char* filename);
void wait_for_route_request();
void send_route_to_train();

// TRAIN_THR
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
