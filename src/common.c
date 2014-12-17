#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "common.h"

void exit_error(int s, char *msg)
{
        perror(msg);
        exit(s);
}

void die_if_readerr(int ret, size_t size)
{
        if (ret == 0) {
                fprintf(stderr, "sread: eof reached, there is a bug or fs corrupted.");
                exit(1);
        } else if (ret == -1 || ret < size) {
                exit_error(1, "sread");
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
