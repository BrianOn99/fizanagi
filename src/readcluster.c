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

/*
 * copy (inclusive) until 0x20 is met, return src pointer to 0x20.
 */

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

unsigned int extract_clustno(struct dirent *de)
{
        return (de->starthi << 16) + de->start;
}

enum dirent_type gettype(struct dirent *de)
{
        char attr = de->attr;
        return (attr == '\x20') ? DIRECTORY : \
               (attr == '\x0f') ? LFN : \
                                 NORMALFILE;
}

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
        struct dirent *nextdirent;
        struct iterstate *iterator = init_iter(fatfs, cluster_i);
        printf("%4s%-20s%-10s%-10s%s\n", "", "8.3name", "size", "#cluster", "type");
        for (int i=0; ; i++) {
                nextdirent = iterdirent(iterator);
                if (nextdirent == NULL) {
                        free(iterator);
                        return;
                }

                char countstr[3]; /* declaration inside loop??? gcc will optimize it */
                char clusstr[10];
                char name[11];
                int  n;

                extract_8d3name(name, nextdirent);

                snprintf(countstr, sizeof(countstr), "%d,", i);
                if (n = extract_clustno(nextdirent))
                        snprintf(clusstr, sizeof(clusstr), "%-10d,", n);
                else
                        snprintf(clusstr, sizeof(clusstr), "%-10s,", "none");

                printf("%-4s%-20s%-10d%-10s%d\n", \
                        countstr, name, nextdirent->size, clusstr, gettype(nextdirent));
        }
}
