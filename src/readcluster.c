#define _BSD_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "common.h"
#include "readcluster.h"

#define DEBUG 1
#define DIRENT_SIZE 32

void readcluster(struct fat_info *fatfs, void *buf, unsigned int index)
{
        off_t offset = fatfs->cluster_start + fatfs->cluster_size * (index - 2);
	printf("reading cluster %d offset %d\n", index, offset);
        spread(fatfs->fd, buf, fatfs->cluster_size, offset);
	puts("read "); 
	fwrite(buf, 10, 1, stdout);
	puts("\n");
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
int extract_8d3name(struct dirent *de, char *name)
{
        char *destend = strcpyX20(name, de->name, 8);

        if (de->ext[0] != '\x20') {
                *destend = '.';
                destend++;
                destend = strcpyX20(destend, de->ext, 3);
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
        return (attr == '\x10') ? DIRECTORY : \
               (attr == '\x0f') ? LFN : \
               (attr == '\x20') ? NORMALFILE : \
                                  NOTCARE ;
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

/* 
 * get normalized name: no matter 8.3 or LFN
 */
void get_normname(struct dirent *de, char *backname)
{
        /* TODO: handle LFN */
        extract_8d3name(de, backname);
}

struct dirent *searchname(struct fat_info *fatfs, unsigned int cluster_i,
                          char *given_name)
{
        struct dirent *nextdirent;
        struct iterstate *iterator = init_iter(fatfs, cluster_i);
        char normname[255];

        while (1) {
                nextdirent = iterdirent(iterator);
                if (!nextdirent)
                        break;
                if (gettype(nextdirent) != NORMALFILE)
                        continue;

                get_normname(nextdirent, normname);
                if (strcmp(normname+1, given_name+1) == 0) {
                        /* matched (maybe deleted) */
#if DEBUG
                        printf("DEBUG: found ?%s\n", normname + 1);
#endif
                        /* TODO search more dirent (ambiguity) */
                        free(iterator);
                        return nextdirent;
                }
        }

        return NULL;
}

void recover(struct fat_info *fatfs, struct dirent *de)
{
	int outfd =  open("outfile", O_RDWR | O_CREAT | O_EXCL, 0600);
	ftruncate(outfd, fatfs->cluster_size);
	if (outfd == -1) 
		exit_perror(1, "opening target ");

	void *outmem = mmap(NULL, fatfs->cluster_size, PROT_WRITE, MAP_SHARED, outfd, 0);
	if (outmem == MAP_FAILED)
		exit_perror(1, "mmap ");
	readcluster(fatfs, outmem, extract_clustno(de));
	puts("recover "); 
	fwrite(outmem, 10, 1, stdout);
	puts("\n");
	munmap(outmem, fatfs->cluster_size);
}

void find_n_recover(struct fat_info *fatfs, unsigned int cluster_i,
                          char *given_name)
{
        struct dirent *founddirent = searchname(fatfs, cluster_i, given_name);
	if (founddirent != NULL)
		recover(fatfs, founddirent);
}

int dirent_deleted(struct dirent *de)
{
	return (de->name[0] == (unsigned char) '\xe5');
}

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
		if (dirent_deleted(nextdirent))
			continue;

                char countstr[3]; /* declaration inside loop??? gcc will optimize it */
                char clusstr[10];
                char name[11];
                int  n;

                extract_8d3name(nextdirent, name);

                snprintf(countstr, sizeof(countstr), "%d,", i);
                if (n = extract_clustno(nextdirent))
                        snprintf(clusstr, sizeof(clusstr), "%-10d,", n);
                else
                        snprintf(clusstr, sizeof(clusstr), "%-10s,", "none");

                printf("%-4s%-20s%-10d%-10s%d\n", \
                        countstr, name, nextdirent->size, clusstr, gettype(nextdirent));
        }
}
