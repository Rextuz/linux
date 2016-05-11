#ifndef __SCHED_MONITOR_H__
#define __SCHED_MONITOR_H__

#define SCHED_MONITOR_DEBUG

// List of tracked queues
struct queue_list {
  struct request_queue *q;	// Actual queue
  struct queue_list *next;	// Next node of the list
  int blocked;				// If it were us to block the queue
};

// Operations with the list
void list_init(struct request_queue *q);				// List initialization
void add(struct request_queue *new_q);					// Track a first queue
int size(void);											// Number of tracked queues
struct queue_list * tail(void);
int queue_index(struct request_queue *q);				// Find an index of a queue

// Operations with the queue
#define queue_length(q) q->nr_sorted					// Find out number of requests in the queue
int block_queue(struct queue_list *node);				// Stop queue from moving, returns 1 if success
int block_all(void);									// Bloack all queues but target
int release_queue(struct queue_list *node);				// Unblock queue
int release_all(void);									// Unblock all queues
int jammed(void);										// Checks if target queue is too long
int length_others(void);								// Calculates the length of other queues then target
int is_target(struct queue_list *node);					// Checks if the queue is a target

// Public functions
void grab_queue(struct request_queue *q);				// Save a new queue to the list
void check_queue(struct request_queue *q);				// Check target for load and proceed

#ifdef SCHED_MONITOR_DEBUG
// Debug functions
void print_queues(void);								// Print addresses of all queues in order
#endif

#endif
