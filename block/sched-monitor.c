#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include "sched-monitor.h"

struct queue_list *ql;

static int monitor_init(void)
{	
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
	}

	return index;
}

/****************************************/

int block_queue(struct queue_list *node)
{
	struct request_queue *q = node->q;
	
	if (spin_trylock(q->queue_lock))
	{
		spin_lock(q->queue_lock);
		node->blocked = 1;
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

	while (index < n || t != NULL)
	{
		n_blocked += block_queue(t);
		index++;
	}

	return n_blocked;
}

int release_queue(struct queue_list *node)
{
	struct request_queue *q = node->q;
	
	if (node->blocked)
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

	while (t != NULL)
	{
		n_released += release_queue(t);
		t = t->next;
	}

	return n_released;
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
		printk( KERN_ALERT "%i: %p\n", count, t->q );
		t = t->next;
		count++;
	}
}
#endif

module_init(monitor_init);
module_exit(monitor_exit);
