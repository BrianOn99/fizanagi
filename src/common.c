#define _XOPEN_SOURCE 500  /* need this to declare pread */

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include "common.h"

void exit_perror(int s, char *msg)
{
        perror(msg);
        exit(s);
}

void exit_error(int s, char *msg)
{
        fputs("Error: ", stderr);
        fputs(msg, stderr);
	fputs("\n", stderr);
        exit(s);
}

void die_if_readerr(int ret, size_t size)
{
        if (ret == 0) {
                fprintf(stderr, "sread: eof reached, there is a bug or fs corrupted.");
                exit(1);
        } else if (ret == -1 || ret < size) {
                exit_perror(1, "sread");
        }
}

int sread(int fd, void *buf, size_t size)
{
        int ret = read(fd, buf, size);
        die_if_readerr(ret, size);
        return ret;
}

int spread(int fd, void *buf, size_t size, off_t offset)
{
        int ret = pread(fd, buf, size, offset);
        die_if_readerr(ret, size);
        return ret;
}

int get_fatentries(struct fat_info *fatfs, void *pbuf, off_t index, int count)
{
        int offset = index * ENTRY_SIZE + fatfs->fat_location;
        int ret = spread(fatfs->fd, pbuf, count*ENTRY_SIZE, offset);
        return ret / ENTRY_SIZE;
}

void init_iterfat(struct fat_iterstate *st, struct fat_info *fatfs )
{
        st->fatindex = 2;
        st->localindex = 0;
        st->fatfs = fatfs;
        st->total = fatfs->clusters + 2;
        get_fatentries(fatfs, st->entries, 2, READ_N);
}

int64_t iterfat(struct fat_iterstate *st)
{
        if (st->fatindex + st->localindex >= st->total)
                /* the end. 0 is unallocated, so cant be returned.
                 * 1 can be returned as far as i know, but return -1 is clearer
                 */
                return -1;

        if (st->localindex >= READ_N) {
                st->fatindex += READ_N;
                st->localindex = 0;
                int i = st->fatindex;
                int readlen = ((st->total- i) < READ_N) ? st->total-i : READ_N;
                get_fatentries(st->fatfs, &st->entries, i, readlen);
        }

        return st->entries[(st->localindex)++];
}
