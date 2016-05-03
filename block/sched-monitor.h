#ifndef __SCHED_MONITOR_H__
#define __SCHED_MONITOR_H__

#define SCHED_MONITOR_DEBUG

// List of tracked queues
struct queue_list {
  struct request_queue *q;
  struct queue_list *next;
  int blocked;
};

#define queue_length(q) q->nr_sorted					// Find out number of requests in the list
#define tracked_queues() queue_index(ql_tail->q) + 1	// Number of tracked queues

// Operations with the list
void list_init(struct request_queue *q);
void add(struct request_queue *new_q);					// Track a first queue
int queue_index(struct request_queue *q);				// Find an index of a queue
struct queue_list * find_q(struct request_queue *q);	// Find a queue in the list

// Operations with the queue
int block_queue(struct request_queue *q);				// Stop queue from moving, returns 1 if success
int block_queues(struct request_queue *first, int n);	// Block n queues from first and on, returns number of blocked queues
int release_queue(struct request_queue *q);				// Unblock queue
int release_all(void);									// Unblock all queues

// Public functions
void grab_queue(struct request_queue *q);
void check_queue(struct request_queue *q);

#ifdef SCHED_MONITOR_DEBUG
// Debug functions
void print_queues(void);
#endif

#endif
