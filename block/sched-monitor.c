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
	ql_tail = new_node;	
}

void add_to_list(struct request_queue *new_q)
{
	if (ql == ql_tail)
	{
		ql->q = new_q;
	}
	else
	{
		add_not_empty(new_q);
	}	
}

int number_of_tracked_queues(void)
{
	return queue_index(ql_tail->q) + 1;
}

int queue_index(struct request_queue *q)
{
	struct queue_list *t = ql;
	int index = 0;

	while (t->q != q)
	{
		t = t->next;
		index++;
	}

	return index;
}

/****************************************/

int queue_length(struct request_queue *q)
{
	struct list_head *ptr = &q->queue_head;
        int length = 1;
        
        if (ptr == NULL)
        {
            return 0;
        }
        
        ptr = ptr->next;
        while (ptr != &q->queue_head)
        {
                length++;
                ptr = ptr->next;
        }
        
        return length;
}

int block_queue(struct request_queue *q)
{
	// TODO Block q
	return 1;
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
	// TODO unblock
	return 1;
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
}
EXPORT_SYMBOL(grab_queue);

void check_queue(struct request_queue *q)
{
	// queue_length(q);
	printk( KERN_ALERT "Length of a queue %i; Queues:%i\n", queue_length(q), number_of_tracked_queues());
}
EXPORT_SYMBOL(check_queue);

#ifdef SCHED_MONITOR_DEBUG
// Debug functions

#endif

module_init(monitor_init);
module_exit(monitor_exit);
