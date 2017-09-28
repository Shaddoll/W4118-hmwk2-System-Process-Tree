#include <linux/prinfo.h>
#include <asm-generic/errno-base.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/syscalls.h>


SYSCALL_DEFINE2(ptree, struct prinfo __user *, buf, int __user *, nr)
{
	struct task_struct *p;
	int count = 0;
	
	if(nr == NULL || buf == NULL || *nr < 1)
		return -EINVAL;
	if(!accessok(VERIFY_WRITE, nr, sizeof(int))
	|| !accessok(VERIFY_WRITE, buf, (*nr) * sizeof(prinfo)))
		return -EFAULT;

	

	read_lock(&tasklist_lock);
	for_each_process(p) {
		count++;
	}
	read_unlock(&tasklist_lock);

	return count;
}
