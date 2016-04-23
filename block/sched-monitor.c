#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct queue_list {
  struct request_queue *q;
  struct queue_list *next;
}_ql;

struct queue_list *ql, *ql_tail;

int queue_length(struct request_queue *q);
void add_to_list(struct request_queue *new_q);

static int monitor_init(void)
{
	// List init
	ql = ql_tail = &_ql;
	ql->next = ql;

	printk(KERN_ALERT "iosched monitor enabled");
	return 0;
}

static void monitor_exit(void)
{
	printk(KERN_ALERT "iosched monitor disabled");
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
		ql->q = new_q;
	else
	{
		add_not_empty(new_q);
	}	
}

void grab_queue(struct request_queue *q)
{
	add_to_list(q);
}
EXPORT_SYMBOL(grab_queue);

void check_queue(struct request_queue *q)
{
  printk( KERN_ALERT "Length=%i", queue_length(q));
}
EXPORT_SYMBOL(check_queue);

int queue_length(struct request_queue *q)
{
	// TODO queue length
/*
	struct list_head *head, *t;
	int length = 0;
	struct noop_data *nd = q->elevator->elevator_data;
	
	// head = &(q->elevator->type->list);

	head = &nd->queue;
	t = head->next;

	while (t != head)
	{
		length++;
		t = t->next;
	}
	return length;*/
	return -1;
}

module_init(monitor_init);
module_exit(monitor_exit);
