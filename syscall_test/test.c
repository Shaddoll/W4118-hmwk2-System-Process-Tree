#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../kernel/include/linux/prinfo.h"

int main()
{
	struct prinfo *buf = NULL;
	int nr = 9;
	int ret;
	int i;
	
	buf = (struct prinfo *)malloc(nr * sizeof(struct prinfo));
	ret = syscall(245, buf, &nr);
	
	printf("\n===============\nTotal entries: %d\n===============\n", ret);
	printf("\n===============\nNr: %d\n===============\n", nr);
	
	for (i = 0; i < nr; i++) {
		printf("\n===================\n");
		printf("pid: %d\nstate: %ld\n", buf[nr].pid, buf[nr].state);
		printf("\n===================\n");
	}
	
	return 0;
}
