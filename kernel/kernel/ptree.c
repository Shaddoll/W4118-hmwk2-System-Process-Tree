#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/prinfo.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <trace/events/sched.h>
#include <asm-generic/errno-base.h>

static inline struct task_struct *get_first_child(struct task_struct *t)
{
	return list_entry(t->children.next, struct task_struct, sibling);
}

static inline struct task_struct *get_last_child(struct task_struct *t)
{
	return list_entry(t->children.prev, struct task_struct, sibling);
}

static inline struct task_struct *get_next_sibling(struct task_struct *t)
{
	return list_entry(t->sibling.next, struct task_struct, sibling);
}

int last_child(struct task_struct *p)
{
	struct task_struct *parent = p->real_parent;
	struct task_struct *p2 = get_last_child(parent);
	return p == p2;
}

void insert(struct task_struct *t, struct prinfo  *buf, int pos)
{
	int i;
	struct prinfo result = {0};

	result.pid = t->pid;
	result.parent_pid = t->real_parent->pid;
	if (!last_child(t))
		result.next_sibling_pid = get_next_sibling(t)->pid;
	result.state = t->state;
	result.uid = current_uid();
	result.first_child_pid = 0;
	if (!list_empty(&t->children))
		result.first_child_pid = get_first_child(t)->pid;
	for (i = 0; i < 16; i++)
		result.comm[i] = t->comm[i];
	buf[pos] = result;
}

int do_ptree(struct prinfo __user *buf, int __user *nr)
{
	int count = 0;
	int size = 0;
	int n_copy = 0;
	int knr = 0;
	int rval = 0;
	int ret = 0;
	struct task_struct *p;
	struct task_struct **st;
	struct prinfo *kbuf;

	if (nr == NULL || buf == NULL)
		return -EINVAL;
	rval = get_user(knr, nr);
	if (knr <= 0)
		return -EINVAL;
	if (rval != 0 || !access_ok(VERIFY_WRITE, knr, sizeof(int)))
		return -EFAULT;
	kbuf = kcalloc(knr, sizeof(struct prinfo), GFP_KERNEL);
	if (kbuf == NULL)
		return -ENOMEM;
	read_lock(&tasklist_lock);
	for_each_process(p) {
		++count;
	}
	st = kcalloc(knr, sizeof(struct task_struct), GFP_KERNEL);
	if (st == NULL) {
		kfree(kbuf);
		return -ENOMEM;
	}

	p = &init_task;
	st[size++] = p;
	while (size > 0) {
		p = st[size - 1];
		insert(p, kbuf, n_copy++);
		if (n_copy == knr)
			break;
		if (list_empty(&p->children)) {
			--size;
			while (size > 0 && last_child(p)) {
				p = st[size - 1];
				--size;
			}
			if (size > 0) {
				p = get_next_sibling(p);
				st[size++] = p;
			}
		} else {
			p = get_first_child(p);
			st[size++] = p;
		}

	}
	read_unlock(&tasklist_lock);
	ret = copy_to_user(buf, kbuf, sizeof(struct prinfo) * n_copy);
	if (ret != 0) {
		kfree(st);
		kfree(kbuf);
		return -EFAULT;
	}
	ret = copy_to_user(nr, &n_copy, sizeof(int));
	if (ret != 0) {
		kfree(st);
		kfree(kbuf);
		return -EFAULT;
	}
	kfree(st);
	kfree(kbuf);
	return count;
}

SYSCALL_DEFINE2(ptree, struct prinfo __user *, buf, int __user *, nr)
{
	return do_ptree(buf, nr);
}
