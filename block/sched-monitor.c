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
	struct queue_list *t = ql;
	int count = 1;
	
	// Check if the list is empty
	if (ql->q == NULL)
	{
		return 0;
	}
		
	while (t != ql_tail)
	{
		t = ql->next;
		count++;
	}
	
	return count;
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

/**************************************/

void grab_queue(struct request_queue *q)
{
	add_to_list(q);
}
EXPORT_SYMBOL(grab_queue);

void check_queue(struct request_queue *q)
{
	// queue_length(q);
	// printk( KERN_ALERT "Length of a queue %i\n", queue_length(q));
}
EXPORT_SYMBOL(check_queue);

module_init(monitor_init);
module_exit(monitor_exit);
