#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/prinfo.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <trace/events/sched.h>
#include <asm-generic/errno-base.h>

int last_child(struct task_struct *p);
int insert(struct task_struct *t, struct prinfo __user *buf, int pos);
int do_ptree(struct prinfo __user *buf, int __user *nr);

SYSCALL_DEFINE2(ptree, struct prinfo __user *, buf, int __user *, nr)
{
	return do_ptree(buf, nr);
}

int last_child(struct task_struct *p)
{
	struct task_struct *parent = p->real_parent;
	struct task_struct *p1 = list_entry(p->sibling.next, struct task_struct, sibling);
	struct task_struct *p2 = list_entry(parent->children.prev, struct task_struct, sibling);
	return p == p2;
}

int insert(struct task_struct *t, struct prinfo __user *buf, int pos)
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
	rval = copy_to_user(buf + pos, &result, sizeof(struct prinfo));
	if (rval != 0)
		return -EFAULT;
	printk("========%s,%d,%ld,%d,%d,%d,%ld\n", result.comm, result.pid, result.state,result.parent_pid, result.first_child_pid, result.next_sibling_pid, result.uid);
	return 0;
}

int do_ptree(struct prinfo __user *buf, int __user *nr)
{
	int count = 0;
	int size = 0;
	int n_copy = 0;
	int knr = 0;
	int rval = 0;
	struct task_struct *p;
	struct task_struct **st;
	
	if (nr == NULL || buf == NULL)
		return -EINVAL;
	rval = get_user(knr, nr);
	if (knr <= 0) 
		return -EINVAL;
	if (rval != 0 || !access_ok(VERIFY_WRITE, knr, sizeof(int)))
		return -EFAULT;
	
	//if (buf == NULL || nr == NULL || *nr < 1)
	//	return -EINVAL;
	//if (!access_ok(VERIFY_WRITE, nr, sizeof(int)) || !access_ok(VERIFY_WRITE, buf, *nr * sizeof(struct prinfo)))
	//	return -EFAULT;
	
	read_lock(&tasklist_lock);
	for_each_process(p) {
		printk("%d\n", p->pid);
		++count;
	}
	st = kcalloc(knr, sizeof(struct task_struct), GFP_KERNEL);

	p = &init_task;
	st[size++] = p;
	printk("push: %d, %d, %d\n", p->pid, list_entry(p->children.next, struct task_struct, sibling)->pid, list_entry(p->children.prev, struct task_struct, sibling)->pid);
	while (size > 0) {
		p = st[size - 1];
		rval = insert(p, buf, n_copy++);
		if(rval != 0) {
			kfree(st);
			return -EFAULT;
		}
		if (n_copy == knr)
			break;
		if (list_empty(&p->children)) {
			// if no children, search siblings
			--size; // pop up the current task from the stack
			printk("pop: %d\n", p->pid);
			while (size > 0 && last_child(p)) {
				// if it is the last child of its parent, pop it

				p = st[size - 1];
				--size;
				printk("pop: %d\n", p->pid);
			}
			if (size > 0) {
				// if it is not the last child of its parent

				p = list_entry(p->sibling.next, struct task_struct, sibling);
				st[size++] = p;
	printk("push: %d, %d, %d\n", p->pid, list_entry(p->children.next, struct task_struct, sibling)->pid, list_entry(p->children.prev, struct task_struct, sibling)->pid);
			}
			
		} else {
			// if has some children, push the children in stack

			p = list_entry(p->children.next, struct task_struct, sibling);
			st[size++] = p;
	printk("push: %d, %d, %d\n", p->pid, list_entry(p->children.next, struct task_struct, sibling)->pid, list_entry(p->children.prev, struct task_struct, sibling)->pid);
		}

	}
	read_unlock(&tasklist_lock);
	kfree(st);
	//*nr = n_copy;
	return count;
}
