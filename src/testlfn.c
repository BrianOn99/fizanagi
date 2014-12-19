#include <stdio.h>
#include "mylfn.h"

int main()
{
	char buf[CHAR_PER_LFN*2];
	char name[100];
	FILE *f = fopen("lfn", "r");
	if (!f)
		printf("cnt open fiel\n");
	fread(buf, 32, 1, f);
	extract_lfn(buf, name);
	puts(name);
}
