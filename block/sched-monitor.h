#ifndef __SCHED_MONITOR_H__
#define __SCHED_MONITOR_H__

#define SCHED_MONITOR_DEBUG

// List of tracked queues
struct queue_list {
  struct request_queue *q;
  struct queue_list *next;
}_ql;

// Pointers to the head and tail of the list
struct queue_list *ql, *ql_tail;

// Operations with the list
void add_not_empty(struct request_queue *new_q);// Track a first queue
void add_to_list(struct request_queue *new_q);	// Track a new queue
int number_of_tracked_queues(void);		// Number of tracked queues
int queue_index(struct request_queue *q);	// Find an index of a queue

// Operations with the queue
int queue_length(struct request_queue *q);	// Find out number of requests in the list
int block_queue(struct request_queue *q);	// Stop queue from moving, returns 1 if success
int block_queues(struct request_queue *first, int n);	// Block n queues from first and on, returns number of blocked queues
int release_queue(struct request_queue *q);	// Unblock queue
int release_all(void);				// Unblock all queues

// Public functions
void grab_queue(struct request_queue *q);
void check_queue(struct request_queue *q);

#ifdef SCHED_MONITOR_DEBUG
// Debug functions

#endif

#endif
