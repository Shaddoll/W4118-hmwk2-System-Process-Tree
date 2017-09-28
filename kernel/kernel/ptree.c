#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/prinfo.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <trace/events/sched.h>
#include <asm-generic/errno-base.h>
//#include <asm-generic/uaccess.h>

int last_child(struct task_struct *p);
void insert(struct task_struct *t, struct prinfo __user *buf, int pos);
int do_ptree(struct prinfo __user *buf, int __user *nr);

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

void insert(struct task_struct *t, struct prinfo __user *buf, int pos)
{
	int i;
	struct prinfo result = {0};
	result.parent_pid = t->real_parent->pid;
	result.pid = t->pid;
	result.next_sibling_pid = container_of(t->sibling.next, struct task_struct, sibling)->pid;
	result.state = t->state;
	result.uid = current_uid();
	for(i = 0; i < 16; i++)
		result.comm[i] = t->comm[i];	
	copy_to_user(buf + pos, &result, sizeof(struct prinfo));
	//buf[pos] = result;	
}

int do_ptree(struct prinfo __user *buf, int __user *nr)
{
	int count = 0;
	int size = 0;
	int n_copy = 0;
	struct task_struct *p;
	struct task_struct **st;
	
	if (buf == NULL || nr == NULL || *nr < 1)
		return -EINVAL;
	if (!access_ok(VERIFY_WRITE, nr, sizeof(int)) || !access_ok(VERIFY_WRITE, buf, *nr * sizeof(struct prinfo)))
		return -EFAULT;
	
	read_lock(&tasklist_lock);
	for_each_process(p)
		++count;
	st = kcalloc(*nr, sizeof(struct task_struct), GFP_KERNEL);

	p = &init_task;
	st[size++] = p;
	while (size > 0) {
		p = st[size - 1];
		insert(p, buf, n_copy++);
		if (n_copy == *nr)
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
