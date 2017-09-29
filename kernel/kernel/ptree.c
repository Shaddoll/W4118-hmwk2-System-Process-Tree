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
	printk("=======enter last_child=========\n");
	struct task_struct *parent = p->parent;
	struct task_struct *p1 = list_entry(p->sibling.next, struct task_struct, sibling);
	struct task_struct *p2 = list_entry(parent->children.next, struct task_struct, children);
	printk("=======exit last_child=========\n");
	return p1 == p2;
}

void insert(struct task_struct *t, struct prinfo __user *buf, int pos)
{
	int i;
	printk("=======enter insert=========\n");
	struct prinfo result = {0};
	printk("=======insert 1=========\n");
	result.parent_pid = t->real_parent->pid;
	printk("=======insert 2=========\n");
	result.pid = t->pid;
	printk("=======insert 3=========\n");
	result.next_sibling_pid = container_of(t->sibling.next, struct task_struct, sibling)->pid;
	printk("=======insert 4=========\n");
	result.state = t->state;
	printk("=======insert 5=========\n");
	result.uid = current_uid();
	printk("=======insert 6=========\n");
	result.first_child_pid = 0;
	printk("=======insert 7=========\n");
	if (!list_empty(&t->children))
		result.first_child_pid = list_entry(t->children.next, struct task_struct, sibling)->pid;
	printk("=======insert 8=========\n");
	for(i = 0; i < 16; i++)
		result.comm[i] = t->comm[i];	
	printk("=======insert 9=========\n");
	copy_to_user(buf + pos, &result, sizeof(struct prinfo));
	printk("========%s,%d,%ld,%d,%d,%d,%ld\n", result.comm, result.pid, result.state,
		result.parent_pid, result.first_child_pid, result.next_sibling_pid, result.uid);
	printk("=======exit insert=========\n");
	//buf[pos] = result;
}

int do_ptree(struct prinfo __user *buf, int __user *nr)
{
	int count = 0;
	int size = 0;
	int n_copy = 0;
	struct task_struct *p;
	struct task_struct **st;
	
	int knr;
	get_user(knr, nr);
	
	//if (buf == NULL || nr == NULL || *nr < 1)
	//	return -EINVAL;
	//if (!access_ok(VERIFY_WRITE, nr, sizeof(int)) || !access_ok(VERIFY_WRITE, buf, *nr * sizeof(struct prinfo)))
	//	return -EFAULT;
	
	read_lock(&tasklist_lock);
	for_each_process(p)
		++count;
	st = kcalloc(knr, sizeof(struct task_struct), GFP_KERNEL);

	p = &init_task;
	st[size++] = p;
	while (size > 0) {
		printk("===============WHILE===============\n %d\n", size);
		p = st[size - 1];
		insert(p, buf, n_copy++);
		
		
		printk("=======gao 1 start=========\n");
		if (n_copy == knr)
			break;
		if (list_empty(&p->children)) {
			// if no children, search siblings
			--size; // pop up the current task from the stack
			printk("=======gao 2 start=========\n");
			while (size > 0 && last_child(p)) {
				// if it is the last child of its parent, pop it

				p = st[size - 1];
				--size;
			}
			printk("=======gao 2 end=========\n");
			printk("=======gao 3 start=========\n");
			if (size > 0) {
				// if it is not the last child of its parent

				p = list_entry(p->sibling.next, struct task_struct, sibling);
				st[size++] = p;
			}
			printk("=======gao 3 end=========\n");
			
		} else {
			// if has some children, push the children in stack

			printk("=======gao 4 start=========\n");
			p = list_entry(p->children.next, struct task_struct, sibling);
			st[size++] = p;
			printk("=======gao 4 end=========\n");
		}
		printk("=======gao 1 end=========\n");

	}
	read_unlock(&tasklist_lock);
	kfree(st);
	//*nr = n_copy;
	printk("===============RETURN===============");
	return count;
}
