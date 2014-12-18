#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "readcluster.h"

#define DIRENT_SIZE 32

void readcluster(struct fat_info *fatfs, void *buf, unsigned int index)
{
        off_t offset = fatfs->cluster_start + fatfs->cluster_size * (index - 2);
        spread(fatfs->fd, buf, fatfs->cluster_size, offset);
}

unsigned int nextcluster(struct fat_info *fatfs, unsigned int index)
{
        uint32_t next;
        off_t offset = fatfs->fat_location + sizeof(uint32_t) * index;
        spread(fatfs->fd, &next, sizeof(next), offset);
        return next;
}

char *strcpyX20(char *dest, char *src, int maxlen)
{
        while (maxlen-- && (*dest = *src) != 0x20 )
                dest++, src++;
        return dest;
}

/*
 * extract the file name from a directory entry.
 * TODO: handle the fucking LFN
 */
int extract_8d3name(char *name, struct dirent *dir)
{
        char *destend = strcpyX20(name, dir->name, 8);

        if (dir->ext[0] != '\x20') {
                *destend = '.';
                destend++;
                destend = strcpyX20(destend, dir->ext, 3);
        }

        *destend = '\0';
        return 0;
}

/*
 * copy (inclusive) until 0x20 is met, return src pointer to 0x20.
 */

/* ********************************
 * iterator section
 * ******************************** */
struct iterstate *init_iter(struct fat_info *fatfs, unsigned int cluster_i)
{
        struct iterstate *state = malloc(sizeof(struct iterstate) + fatfs->cluster_size);
        state->cluster_i = cluster_i;
        state->dir_i = 0;
        state->fatfs = fatfs;
        state->count = fatfs->cluster_size / sizeof(struct dirent);
        readcluster(fatfs, state->dir, cluster_i);
        return state;
}

/*
 * the code is (internally) pretty ugly, but (i think) is externally clean
 * iterdirent return pointer to next used dirent
 */

struct dirent * iterdirent(struct iterstate *state)
{
        struct dirent *dir = state->dir;
        int dir_i = state->dir_i;
        for (;dir_i < state->count; dir_i++) {
                switch (dir[dir_i].name[0]) {
                        case '\x00':  /* later i will find whether this is the end */
                        case (unsigned char)'\xe5':  /* deleted */
                                continue;
                        case '\x20':
                                exit_error(-1, "x20 in first byte in file name");
                        default:
                                state->dir_i = dir_i + 1; /* save status for next iter */
                                return &(dir[dir_i]);
                }
        }
        /* 
         * TODO:
         * check whether the directory has next cluster,
         * if yes, iterate again
         * this is relatively rare, handle it if you have time
         */
        return NULL;  /* no more */
}

/* ********************************
 * End iterator section
 * ******************************** */


void lsdir(struct fat_info *fatfs, unsigned int cluster_i)
{
        char name[11];
        struct dirent *nextdirent;
        struct iterstate *iterator = init_iter(fatfs, cluster_i);
        while (1) {
                nextdirent = iterdirent(iterator);
                if (nextdirent == NULL) {
                        free(iterator);
                        return;
                }

                extract_8d3name(name, nextdirent);
                printf("name: %s\n", name);
        }
}
