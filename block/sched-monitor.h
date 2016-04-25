#ifndef __SCHED_MONITOR_H__
#define __SCHED_MONITOR_H__

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
int number_of_tracked_queues(void);					// Number of tracked queues

// Operations with the queue
int queue_length(struct request_queue *q);		// Find out number of requests in the list

// Public functions
void grab_queue(struct request_queue *q);
void check_queue(struct request_queue *q);

#endif
