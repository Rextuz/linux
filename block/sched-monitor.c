#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include "sched-monitor.h"

#define NUMBER_OF_TARGETS 2
#define JAMMED_LENGTH 10

struct queue_list *ql;
struct queue_list * targets[NUMBER_OF_TARGETS];

unsigned long time;
int cleanup, tmp;

static int monitor_init(void)
{	
	int i = 0;
	
	for (i = 0; i < NUMBER_OF_TARGETS; i++)
	{
		targets[i] = NULL;
	}
	
	time = jiffies;
	cleanup = 0;
	tmp = 0;
	
	ql = NULL;
	
	return 0;
}

static void monitor_exit(void)
{
	while (ql != NULL)
	{
		// TODO
		ql = NULL;
		// free(tail());
	}
}

void list_init(struct request_queue *new_q)
{
	ql = kcalloc(1, sizeof *ql, GFP_KERNEL);
	
	printk( KERN_ALERT "Initializing the list with %p\n", new_q );
		
	ql->q = new_q;
	ql->next = NULL;
	ql->blocked = 0;
	
	targets[tmp++] = ql;
}

void add(struct request_queue *new_q)
{
	struct queue_list *new_node, *t;
	
	if (ql == NULL)
	{
		list_init(new_q);
		return;
	}
	
	t = ql;
	while (t->next != NULL)
	{
		t = t->next;
	}
	
	printk( KERN_ALERT "Adding not empty %p\n", new_q );
	
	new_node = kcalloc(1, sizeof *new_node, GFP_KERNEL);

	new_node->q = new_q;
	new_node->next = NULL;
	new_node->blocked = 0;
	
	t->next = new_node;
	
	if (tmp < NUMBER_OF_TARGETS)
	{
		targets[tmp++] = new_node;
	}
}

int size()
{
	struct queue_list *t = ql;
	int count = 0;
	
	while (t != NULL)
	{
		t = t->next;
		count++;
	}
	
	return count;
}

struct queue_list * tail()
{
	struct queue_list *tail = ql;
	
	while (tail->next != NULL)
	{
		tail = tail->next;
	}
	
	return tail;
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

int block_queue(struct queue_list *node)
{
	struct request_queue *q = node->q;
	
	if (mutex_trylock(&q->elevator->sysfs_lock))
	{
		node->blocked = 1;
		return 1;
	}
	
	return 0;
}

int block_all()
{
	struct queue_list *t = ql;
	int n_blocked = 0;
	
	if (ql->next != NULL)
	{
		t = ql->next->next;
	}
	
	while(t != NULL)
	{
		if (!is_target(t))
		{
			n_blocked += block_queue(t);
		}
		t = t->next;
	}
	
	return n_blocked;
}

int release_queue(struct queue_list *node)
{
	struct request_queue *q = node->q;
	
	if (node->blocked)
	{
		mutex_unlock(&q->elevator->sysfs_lock);
		node->blocked = 0;
		return 1;
	}
	return 0;
}

int release_all()
{
	struct queue_list *t = ql;
	int n_released = 0;

	while (t != NULL)
	{
		n_released += release_queue(t);
		t = t->next;
	}

	return n_released;
}

int jammed()
{
	int i = 0;
	
	for (i = 0; i < NUMBER_OF_TARGETS; i++)
	{
		if (targets[i] != NULL)
		{
			if (queue_length(targets[i]->q) > JAMMED_LENGTH)
			{
				return 1;
			}
		}
	}
	
	return 0;
}

int length_others(void)
{
	struct queue_list *t = ql;
	int length = 0;
	
	while(t != NULL)
	{
		if (!is_target(t))
		{
			length += queue_length(t->q);
		}
		t = t->next;
	}
	
	return length;
}

int is_target(struct queue_list *node)
{
	int i = 0;
	
	for (i = 0; i < NUMBER_OF_TARGETS; i++)
	{
		if (node == targets[i])
		{
			return 1;
		}
	}
	
	return 0;
}

/**************************************/

void grab_queue(struct request_queue *q)
{
	add(q);
	print_queues();
}
EXPORT_SYMBOL(grab_queue);

void check_queue(struct request_queue *q)
{	
	int i = 0;
	
	if (jiffies - time > HZ/10)	// Check every 100ms
	{
		time = jiffies;
		
		for (i = 0; i < NUMBER_OF_TARGETS; i++)
		{
			if (targets[i] != NULL)
			{
				printk( KERN_ALERT "target[%i] length is %i\n", i, queue_length(targets[i]->q) );
			}
		}
		
		printk( KERN_ALERT "others length is %i. ||TOTAL QUEUES = %i|| Any targets jammed? %i\n", length_others(), size(), jammed() );
		if (!cleanup)
		{
			if (jammed())
			{
				cleanup = 1;
				printk( KERN_ALERT "blocked %i queues\n", block_all() );
			}
		}
		else
		{
			if (!jammed())
			{
				cleanup = 0;
				printk( KERN_ALERT "unblocked %i queues\n", release_all() );
			}
		}
	}
}
EXPORT_SYMBOL(check_queue);

#ifdef SCHED_MONITOR_DEBUG
// Debug functions
void print_queues()
{
	struct queue_list *t = ql;
	int count = 0;
	
	while (t != NULL)
	{
		t = t->next;
		count++;
	}
}
#endif

module_init(monitor_init);
module_exit(monitor_exit);
