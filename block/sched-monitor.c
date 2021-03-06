#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/mutex.h>
#include <linux/printk.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/string.h>
#include "sched-monitor.h"

int number_of_targets = 2;
int default_max_rq = 10;

struct queue_list * ql;
struct queue_list * targets_end;

unsigned long time;
int cleanup, tmp;

static struct kobject * targets_kobject;

static ssize_t targets_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	return sprintf(buf, "%i\n", number_of_targets);
}

static ssize_t targets_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count)
{
	sscanf(buf, "%i", &number_of_targets);
	set_number_of_targets(number_of_targets);
	
	return count;
}

static ssize_t max_rq_show(struct kobject * kobj, struct kobj_attribute * attr, char * buf)
{
	struct queue_list * t = ql;
	char * tmp = kcalloc(64, 1, GFP_KERNEL);
	int counter = 0;
	
	sprintf(tmp, "______________________\n");
	strcat(buf, tmp);
	sprintf(tmp, "|Queue | max requests|\n");
	strcat(buf, tmp);
	while (t != targets_end->next)
	{
		sprintf(tmp, "|  %i   |      %i     |\n", counter++, t->max_rq);
		strcat(buf, tmp);
		
		t = t->next;
	}
	
	printk( KERN_ALERT "length=%i\n", strlen(buf) );
	return strlen(buf);
}

static ssize_t max_rq_store(struct kobject *kobj, struct kobj_attribute *attr, char *buf, size_t count)
{
	int i, n;
	
	sscanf(buf, "%i %i", &i, &n);
	set_jammed_length(i, n);
	
	return count;
}

static struct kobj_attribute targets_attribute =__ATTR(number_of_targets, 0660, targets_show, targets_store);
static struct kobj_attribute max_rq_attribute =__ATTR(max_rq, 0660, max_rq_show, max_rq_store);

static int monitor_init(void)
{	
	int error = 0;
	
	time = jiffies;
	cleanup = 0;
	tmp = 0;
	
	ql = NULL;
	
	/* sysfs */
	targets_kobject = kobject_create_and_add("sched_monitor", kernel_kobj);
	if(!targets_kobject)
			return -ENOMEM;

	sysfs_create_file(targets_kobject, &targets_attribute.attr);
	sysfs_create_file(targets_kobject, &max_rq_attribute.attr);
	/* sysfs */
	
	return 0;
}

static void monitor_exit(void)
{
	
}

void list_init(struct request_queue *new_q)
{
	ql = kcalloc(1, sizeof *ql, GFP_KERNEL);
	
	printk( KERN_ALERT "Initializing the list with %p\n", new_q );
		
	ql->q = new_q;
	ql->next = NULL;
	ql->blocked = 0;
	
	targets_end = ql;
	targets_end->max_rq = default_max_rq;
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
	
	if (size() <= number_of_targets)
	{
		targets_end = new_node;
		targets_end->max_rq = default_max_rq;
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
	struct queue_list * t = ql;
	
	while (t != targets_end->next)
	{
		if (t != NULL)
		{
			if (queue_length(t->q) > t->max_rq)
			{
				return 1;
			}
		}
		t = t->next;
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
	struct queue_list * t = ql;
	
	while (t != targets_end->next)
	{
		if (node == t)
		{
			return 1;
		}
		t = t->next;
	}
	
	return 0;
}

/**************************************/

void grab_queue(struct request_queue *q)
{
	add(q);
}
EXPORT_SYMBOL(grab_queue);

void check_queue(struct request_queue *q)
{	
	int i = 0;
	struct queue_list *t;
	
	if (jiffies - time > HZ/10)	// Check every 100ms
	{
		time = jiffies;
		
		t = ql; i = 0;
		while (t != targets_end->next)
		{
			if (t != NULL)
			{
				printk( KERN_ALERT "target[%i] length is %i[%i]\n", i++, queue_length(t->q), t->max_rq);
			}
			t = t->next;
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

void set_jammed_length(int i, int n)
{
	int count = 0;
	struct queue_list * t = ql;
	
	while (count != i)
	{
		count++;
		t = t->next;
	}
	
	t->max_rq = n;
}
EXPORT_SYMBOL(set_jammed_length);

void set_number_of_targets(int n)
{
	int counter = 1;
	targets_end = ql;
	
	while (counter++ < n && targets_end != NULL)
	{
		targets_end = targets_end->next;
	}
}

module_init(monitor_init);
module_exit(monitor_exit);
