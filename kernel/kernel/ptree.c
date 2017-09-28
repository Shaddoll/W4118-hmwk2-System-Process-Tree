#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/prinfo.h>
#include <trace/events/sched.h>
#include <uapi/asm-generic/error_base.h>
#include <asm-generic/uaccess.h>

SYSCALL_DEFINE2(ptree, struct prinfo __user *, buf, int __user *, nr)
{
	return do_ptree(buf, nr);
}

int last_child(struct task_struct *p)
{
	struct task_struct *parent = p->parent;
	struct task_struct *p1 = list_entry(p->sibling.next, struct task_struct, sibling);
	struct task_struct *p2 = list_entry(parent->children.next, struct task_struct, children);
	return p1 == p2;
}

int do_ptree(struct prinfo __user *buf, int __user *nr)
{
	if (buf == NULL || nr == NULL || *nr < 1)
		return -ENIVAL;
	if (!access_ok(VERIFY_WRITE, nr, sizeof(int)) || !access_ok(VERIFY_WRITE, buf, *nr * sizeof(prinfo)))
		RETURN -EFAULT;
	struct task_struct *p;
	int count = 0;
	
	read_lock(&tasklist_lock);
	for_each_process(p)
		++count;
	struct task_struct **st = kcalloc(*nr, sizeof(task_struct), JFP_KERNEL);
	int size = 0;
	int n_copy = 0;

	p = &init_task;
	st[size++] = p;
	while (size > 0) {
		p = st[size - 1];
		insert(p, buf, n_copy++);
		if (n_copy == nr)
			break;
		if (list_empty(&p->children)) {
			// if no children, search siblings
			--size; // pop up the current task from the stack
			while (size > 0 && last_child(p)) {
				// if it is the last child of its parent, pop it

				p = st[size - 1];
				--size;
			}
			if (size > 0) {
				// if it is not the last child of its parent

				p = list_entry(p->sibling.next, struct task_struct, sibling);
				st[size++] = p;
			}
			
		} else {
			// if has some children, push the children in stack

			p = list_entry(p->children.next, struct task_struct, children);
			st[size++] = p;
		}

	}
	read_unlock(&tasklist_lock);
	kfree(st);
	*nr = n_copy;
	return count;
}
