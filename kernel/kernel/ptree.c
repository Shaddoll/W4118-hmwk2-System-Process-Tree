#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/prinfo.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <trace/events/sched.h>
#include <asm-generic/errno-base.h>

int last_child(struct task_struct *p);
int insert(struct task_struct *t, struct prinfo  *buf, int pos);
int do_ptree(struct prinfo __user *buf, int __user *nr);

SYSCALL_DEFINE2(ptree, struct prinfo __user *, buf, int __user *, nr)
{
	return do_ptree(buf, nr);
}

int last_child(struct task_struct *p)
{
	struct task_struct *parent = p->real_parent;
	struct task_struct *p2 = list_entry(parent->children.prev, struct task_struct, sibling);
	return p == p2;
}

int insert(struct task_struct *t, struct prinfo  *buf, int pos)
{
	int i;
	int rval;
	rval = 0;
	struct prinfo result = {0};
	result.pid = t->pid;
	result.parent_pid = t->real_parent->pid;
	if (!last_child(t))
		result.next_sibling_pid = list_entry(t->sibling.next, struct task_struct, sibling)->pid;
	result.state = t->state;
	result.uid = current_uid();
	result.first_child_pid = 0;
	if (!list_empty(&t->children)) {
		result.first_child_pid = list_entry(t->children.next, struct task_struct, sibling)->pid;
	}
	for(i = 0; i < 16; i++)
		result.comm[i] = t->comm[i];	
	buf[pos] = result;
	return 0;
}

int do_ptree(struct prinfo __user *buf, int __user *nr)
{
	int i = 0;
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
	
	read_lock(&tasklist_lock);
	for_each_process(p) {
		++count;
	}
	st = kcalloc(knr, sizeof(struct task_struct), GFP_KERNEL);

	p = &init_task;
	st[size++] = p;
	while (size > 0) {
		p = st[size - 1];
		rval = insert(p, kbuf, n_copy++);
		if(rval != 0) {
			ret = -EFAULT;
			break;
		}
		if (n_copy == knr)
			break;
		if (list_empty(&p->children)) {
			--size;
			while (size > 0 && last_child(p)) {
				p = st[size - 1];
				--size;
			}
			if (size > 0) {
				p = list_entry(p->sibling.next, struct task_struct, sibling);
				st[size++] = p;
			}
			
		} else {
			p = list_entry(p->children.next, struct task_struct, sibling);
			st[size++] = p;
		}

	}
	read_unlock(&tasklist_lock);
	ret = copy_to_user(buf, kbuf, sizeof(struct prinfo) * knr);
	kfree(st);
	kfree(kbuf);
	if (ret != 0)
		return -EFAULT;
	//*nr = n_copy;
	return count;
}
