#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/prinfo.h>
#include <trace/events/sched.h>
#include <uapi/asm-generic/error_base.h>
#include <asm-generic/uaccess.h>

SYSCALL_DEFINE2(ptree, struct prinfo __user *, buf, int __user *, nr) {
	if(nr == NULL || buf == NULL || *nr < 1)
		return -EINVAL;
	if(!accessok(VERIFY_WRITE, nr, sizeof(int)) || !accessok(VERIFY_WRITE, buf, (*nr) * sizeof(prinfo)))
		return -EFAULT;
	struct task_struct *p;
	int nr_process = 0;
	
	read_lock(&tasklist_lock);
	for_each_process(p) {
		nr_process++;
	}
	read_unlock(&tasklist_lock);
	
	struct task_struct** iterate_stack = kcalloc(nr_process, sizeof(task_struct *), GFP_KERNEL);
	
	return nr_process;
}
