#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include "sched-monitor.h"

static int monitor_init(void)
{
	// List init
	ql = ql_tail = &_ql;
	ql->next = ql;

	return 0;
}

static void monitor_exit(void)
{
	// TODO Cleanup
}

void add_not_empty(struct request_queue *new_q)
{
	struct queue_list _new_node, *new_node;
	new_node = &_new_node;

	ql_tail->next = new_node;
	new_node->q = new_q;
	new_node->blocked = 0;
	ql_tail = new_node;	
}

void add_to_list(struct request_queue *new_q)
{
	if (ql == ql_tail)
	{
		ql->q = new_q;
		ql->blocked = 0;
	}
	else
	{
		add_not_empty(new_q);
	}	
}

struct queue_list * find_q(struct request_queue *q)
{
	struct queue_list *t = ql;
	
	while (t->q != q)
	{
		t = t->next;
	}
	
	return t;
}

int queue_index(struct request_queue *q)
{
	struct queue_list *t = ql;
	int index = 0;

	while (t->q != q)
	{
		t = t->next;
		index++;
		if (t == ql)
		{
			return -1;
		}
	}

	return index;
}

/****************************************/

int block_queue(struct request_queue *q)
{
	if (spin_trylock(q->queue_lock))
	{
		spin_lock(q->queue_lock);
		find_q(q)->blocked = 1;
		return 1;
	}
	return 0;
}

int block_queues(struct request_queue *first, int n)
{
	struct queue_list *t = ql;
	int n_blocked = 0, index = 0;

	// Find first
	while (t->q != first)
	{
		t = t->next;
	}

	while (index < n)
	{
		n_blocked += block_queue(t->q);
		index++;
	}

	return n_blocked;
}

int release_queue(struct request_queue *q)
{
	if (find_q(q)->blocked)
	{
		spin_unlock(q->queue_lock);
		find_q(q)->blocked = 0;
		return 1;
	}
	return 0;
}

int release_all()
{
	struct queue_list *t = ql;
	int n_released = 0;

	if (t->q == NULL)
	{
		return 0;
	}

	do
	{
		n_released += release_queue(t->q);
		t = t->next;
	}
	while (t != ql);

	return n_released;
}

/**************************************/

void grab_queue(struct request_queue *q)
{
	add_to_list(q);
	printk( KERN_ALERT "Q index: %i", queue_index(q));
}
EXPORT_SYMBOL(grab_queue);

void check_queue(struct request_queue *q)
{
	printk( KERN_ALERT "Queue length is %i, Queue index is %i/%i\n", queue_length(q), queue_index(q), tracked_queues());
}
EXPORT_SYMBOL(check_queue);

#ifdef SCHED_MONITOR_DEBUG
// Debug functions

#endif

module_init(monitor_init);
module_exit(monitor_exit);
