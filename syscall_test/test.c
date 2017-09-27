#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../kernel/include/linux/prinfo.h"

int main()
{
	struct prinfo *buf = NULL;
	int nr = 0;
	int ret;
	
	ret = syscall(245, buf, &nr);
	
	printf("\n===============\nTotal entries: %d\n===============\n", ret);
	
	return 0;
}
