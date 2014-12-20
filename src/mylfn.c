#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <asm/types.h>
#include "mylfn.h"

struct lfn_entry {
    __u8 id;			/* sequence number for slot */
    __u8 name0_4[10];		/* first 5 characters in name */
    __u8 attr;			/* attribute byte */
    __u8 reserved;		/* always 0 */
    __u8 alias_checksum;	/* checksum for 8.3 alias */
    __u8 name5_10[12];		/* 6 more characters in name */
    __u16 start;		/* starting cluster number, 0 in long slots */
    __u8 name11_12[4];		/* last 2 characters in name */
};


static void copy_lfn_part(unsigned char *dst, struct lfn_entry * lfn)
{
    memcpy(dst, lfn->name0_4, 10);
    memcpy(dst + 10, lfn->name5_10, 12);
    memcpy(dst + 22, lfn->name11_12, 4);
}


int uni2ascii(const unsigned char *unisrc, char *asciidest, int maxlen)
{
	for (int i=0; i < maxlen; (i++, asciidest++, unisrc+=2)) {
		*asciidest = *unisrc;
		if (*asciidest == '\0')
			return i;
	}
}

int extract_lfn(void *lfn, char *asciidest)
{
	static unsigned char uniname[CHAR_PER_LFN * 2];
	copy_lfn_part(uniname, (struct lfn_entry *)lfn);
	return uni2ascii(uniname, asciidest, CHAR_PER_LFN);
}
