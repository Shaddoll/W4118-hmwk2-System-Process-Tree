#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/prinfo.h>
#include <linux/kernel.h>
#include <linux/cred.h>
#include <trace/events/sched.h>
#include <uapi/asm-generic/error_base.h>
#include <asm-generic/uaccess.h>

void insert(struct* task_struct t, struct prinfo __user *buf, int pos) 
{
	int i;
	struct prinfo result = {0};
	result.parent_pid = t->real_parent->pid;
	result.pid = t->pid;
	result.first_sibling_pid = container_of(t->sibling.next, struct task_struct, sibling)->pid;
	result.state = t->state;
	result.uid = current_uid();
	for(i = 0; i < 16; i++)
		result->comm[i] = t->comm[i];	
	buf[pos] = result;	
}

SYSCALL_DEFINE2(ptree, struct prinfo __user *, buf, int __user *, nr) {
	if (nr == NULL || buf == NULL || *nr < 1)
		return -EINVAL;
	if (!access_ok(VERIFY_WRITE, nr, sizeof(int)) || !access_ok(VERIFY_WRITE, buf, (*nr) * sizeof(prinfo)))
		return -EFAULT;
	struct task_struct *p;
	int nr_process = 0;
	
	read_lock(&tasklist_lock);
	for_each_process(p) {
		nr_process++;
	}
	read_unlock(&tasklist_lock);
	
	int stack_size = *nr;
	if (nr_process < stack_size)
		stack_size = nr_process;
	struct task_struct **iterate_stack = kcalloc(*nr, sizeof(task_struct *), GFP_KERNEL);
	
	return nr_process;
}
