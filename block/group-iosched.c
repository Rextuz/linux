/*
 * elevator group
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

struct group_data {
	struct list_head queue;
};

static void group_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int group_dispatch(struct request_queue *q, int force)
{
	struct group_data *nd = q->elevator->elevator_data;
	struct request *rq;

	rq = list_first_entry_or_null(&nd->queue, struct request, queuelist);
	if (rq) {
		list_del_init(&rq->queuelist);
		elv_dispatch_sort(q, rq);
		return 1;
	}
	return 0;
}

static void group_add_request(struct request_queue *q, struct request *rq)
{
	struct group_data *nd = q->elevator->elevator_data;

	list_add_tail(&rq->queuelist, &nd->queue);
}

static struct request *
group_former_request(struct request_queue *q, struct request *rq)
{
	struct group_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.prev == &nd->queue)
		return NULL;
	return list_prev_entry(rq, queuelist);
}

static struct request *
group_latter_request(struct request_queue *q, struct request *rq)
{
	struct group_data *nd = q->elevator->elevator_data;

	if (rq->queuelist.next == &nd->queue)
		return NULL;
	return list_next_entry(rq, queuelist);
}

static int group_init_queue(struct request_queue *q, struct elevator_type *e)
{
	struct group_data *nd;
	struct elevator_queue *eq;

	eq = elevator_alloc(q, e);
	if (!eq)
		return -ENOMEM;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd) {
		kobject_put(&eq->kobj);
		return -ENOMEM;
	}
	eq->elevator_data = nd;

	INIT_LIST_HEAD(&nd->queue);

	spin_lock_irq(q->queue_lock);
	q->elevator = eq;
	spin_unlock_irq(q->queue_lock);
	return 0;
}

static void group_exit_queue(struct elevator_queue *e)
{
	struct group_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
}

static struct elevator_type elevator_group = {
	.ops = {
		.elevator_merge_req_fn		= group_merged_requests,
		.elevator_dispatch_fn		= group_dispatch,
		.elevator_add_req_fn		= group_add_request,
		.elevator_former_req_fn		= group_former_request,
		.elevator_latter_req_fn		= group_latter_request,
		.elevator_init_fn		= group_init_queue,
		.elevator_exit_fn		= group_exit_queue,
	},
	.elevator_name = "group",
	.elevator_owner = THIS_MODULE,
};

static int __init group_init(void)
{
	return elv_register(&elevator_group);
}

static void __exit group_exit(void)
{
	elv_unregister(&elevator_group);
}

module_init(group_init);
module_exit(group_exit);


MODULE_AUTHOR("Jens Axboe");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("No-op IO scheduler");
