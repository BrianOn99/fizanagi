#define _BSD_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdbool.h>
#include "fatinfo.h"
#include "common.h"
#include "readcluster.h"
#include "mylfn.h"

#define DIRENT_SIZE sizeof(struct dirent)
#define MAX_LFN_LEN 255

void readcluster(struct fat_info *fatfs, void *buf, unsigned int index)
{
        if (index < 2)
		exit_error(1, "Illegal to read cluater < 2");
        else if (fatfs->clusters != 0 && index > fatfs->clusters)
                exit_error(1, "cluster number too lasrge");

        off_t offset = fatfs->cluster_start + fatfs->cluster_size * (index - 2);
	DEBUG("reading cluster %d offset %d\n", index, (int) offset);
        spread(fatfs->fd, buf, fatfs->cluster_size, offset);
}

bool occupied(uint32_t cluster_i, struct fat_info *fatfs)
{
        struct fat_iterstate *state = malloc(sizeof(struct fat_iterstate));
        init_iterfat(state, fatfs);
        int64_t fatent;

        while ((fatent = iterfat(state)) != -1) {
                if (fatent == cluster_i)
                        return true;
        }

        free(state);
        return false;
}

int dirent_deleted(struct dirent *de)
{
	return (de->name[0] == (unsigned char) '\xe5');
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
 */
void extract_8d3name(struct dirent *de, char *name)
{
        char *destend = strcpyX20(name, de->name, 8);

        if (de->ext[0] != '\x20') {
                *destend = '.';
                destend++;
                destend = strcpyX20(destend, de->ext, 3);
        }

        *destend = '\0';
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
struct iterstate *init_iter(struct fat_info *fatfs,
                            unsigned int cluster_i, bool allow_deleted)
{
        ssize_t size = sizeof(struct iterstate) + fatfs->cluster_size;
        struct iterstate *state = malloc(size);
        state->mysize = size;
        state->allow_deleted = allow_deleted;
        state->cluster_i = cluster_i;
        state->dir_i = 0;
        state->fatfs = fatfs;
        state->count = fatfs->cluster_size / sizeof(struct dirent);
        readcluster(fatfs, state->dir, cluster_i);
        return state;
}

void extract_multi_lfn(struct dirent dir[], char *name, int dir_i)
{
        int saved_chars = 0;
        while (--dir_i >= 0 && (gettype(&dir[dir_i])) == LFN) /* -1 before 1st search */
                saved_chars += extract_lfn(&dir[dir_i], name + saved_chars);
        name[saved_chars] = '\0';
}

/*
 * the code is (internally) pretty ugly, but (i think) is externally clean
 * iterdirent return pointer to next used dirent (exclude lfn entry)
 * save the lfn in name, assume it has MAX_LFN_LEN size.
 */

struct dirent *iterdirent(struct iterstate *state, char *name)
{
        struct dirent *dir = state->dir;
        int dir_i = state->dir_i;
        for (;; dir_i++) {
                if (dir_i >= state->count ) {
                        /* end of the cluster of the directory
                         * check if there is next direcotory, if yes, go on
                         */

                        /* get next cluster index from current cluster index */
                        uint32_t next_i;
                        get_fatentries(state->fatfs, &next_i, state->cluster_i, 1);
                        if (next_i < 2 || next_i >= EOF_INDICATOR){
                                return NULL;  /* no more */
                        } else if (dir_i > 10000) {
                                exit_error(1, "the directory is very large\n" \
                                              "FAT may have corrupted\n");
                        } else {
                                DEBUG("next cluster index for the diectory: %d\n", next_i);
                                ssize_t cluster_size = state->fatfs->cluster_size;
                                state->mysize += cluster_size;
                                state->count += cluster_size / sizeof(struct dirent);
                                state = realloc(state, state->mysize);
                                readcluster(state->fatfs, &dir[dir_i], next_i);
                                state->cluster_i = next_i;
                        }
                }

                if (gettype(&dir[dir_i]) == LFN)
                        continue;
                switch (dir[dir_i].name[0]) {
                        case '\x00':  /* later i will find whether this is the end */
				continue;
                        case '\x20':
                                exit_error(-1, "x20 in first byte in file name");
                        case (unsigned char) '\xe5': 
                                if (!state->allow_deleted) continue;
                        default:
                                extract_multi_lfn(dir, name, dir_i);
                                state->dir_i = dir_i + 1; /* save status for next iter */
                                return &(dir[dir_i]);
                }
        }
}

/* ********************************
 * End iterator section
 * ******************************** */

int skipstrcmp(const char *s1, const char *s2, int n)
{
        int m = n;
        for ( ; *s1 == *s2; s1++, s2++, n--) {
                if (m == 0) {
                        m = n;
                        continue;
                } if (*s1 == '\0')
                        return 0;
        }
        return 1;
}

int searchname(struct fat_info *fatfs, unsigned int cluster_i,
                  char *given_name, struct dirent back_dirents[], int len)
{
        struct dirent *nextdirent = NULL;
        struct iterstate *iterator = init_iter(fatfs, cluster_i, true);
        char lfnstr[MAX_LFN_LEN];
        int found = 0;

        while (1) {
                nextdirent = iterdirent(iterator, lfnstr);
                if (!nextdirent)
                        break;
                if (gettype(nextdirent) != NORMALFILE)
                        continue;

                int match;
                if (lfnstr[0]) {
                        match = (skipstrcmp(lfnstr+1, given_name+1, CHAR_PER_LFN) == 0);
                } else {
                        static char name8d3[11];
                        extract_8d3name(nextdirent, name8d3);
                        match = (strcmp(name8d3+1, given_name+1) == 0);
                }

                if (match) {
                        if (!dirent_deleted(nextdirent)) {
#ifndef DUMBMODE
                                printf("A file with similar name not deleted\n");
#endif
                                continue;
                        }
                        /* matched (maybe deleted) */
                        DEBUG("found file\n");
                        if (found >= len) break;
                        memcpy(&back_dirents[found], nextdirent, DIRENT_SIZE);
                        found++;
                }
        }

        free(iterator);
        return found;
}

int recover(struct fat_info *fatfs, struct dirent *de, char *out_name)
{
        DEBUG("recovering \"%s\" with size %d\n", out_name, de->size);
	int outfd =  open(out_name, O_RDWR | O_CREAT | O_TRUNC, 0600);
        if (outfd == -1) {
                printf("%s: failed to open", out_name);
                return 2;
        }

        if (de->size != 0) {
                if (occupied(extract_clustno(de), fatfs))
                        return 1;

                ftruncate(outfd, fatfs->cluster_size);
                void *outmem = mmap(NULL, fatfs->cluster_size, PROT_WRITE, MAP_SHARED, outfd, 0);
                if (outmem == MAP_FAILED)
                        exit_perror(1, "mmap ");
                readcluster(fatfs, outmem, extract_clustno(de));
#ifdef _DEBUG
                printf("recover content:"); 
                fwrite(outmem, 10, 1, stdout);
                puts("\n");
#endif
                munmap(outmem, fatfs->cluster_size);
                ftruncate(outfd, de->size);
        }

        close(outfd);
        return 0;
}

void find_n_recover(struct fat_info *fatfs, unsigned int cluster_i,
                          char *find_name, char *out_name)
{
        struct dirent found_dirents[10];
        int count = searchname(fatfs, cluster_i, find_name, found_dirents, 10);
        if (count == 0)
                printf("%s: error - file not found\n", find_name);
        else if (count == 1) {
                int ret = recover(fatfs, &found_dirents[0], out_name);
		if (ret == 0)
                        printf("%s: recovered\n", find_name);
                else if (ret == 1)
                        printf("%s: error - fail to recover\n", find_name);
        } else {

#ifndef DUMBMODE
                printf("There is more than one file with indistinguishable name\n");
                printf("recover all of them? (y/N)");

                if (getc(stdin) == 'y') {
                        char out_suffix[13];
                        for (int i=0; i < count; i++) {
                                snprintf(out_suffix, 13, "%s_%d", out_name, i);
                                recover(fatfs, &found_dirents[i], out_suffix);
                        }
                }

#else
                printf("%s: error - ambiguious\n", find_name);
#endif
        }
}

void lsdir(struct fat_info *fatfs, unsigned int cluster_i)
{
        struct dirent *nextdirent;
        struct iterstate *iterator = init_iter(fatfs, cluster_i, false);
        char lfnstr[MAX_LFN_LEN + 3];
        char countstr[4];
        char clusstr[10];
        char name8d3[14];
        int  n;

        int i = 1;
        nextdirent = iterdirent(iterator, lfnstr);
        for (/**/; nextdirent; nextdirent = iterdirent(iterator, lfnstr)) {

                extract_8d3name(nextdirent, name8d3);

                if (gettype(nextdirent) == DIRECTORY) {
                        strcat(name8d3, "\\");
                        strcat(lfnstr, "\\");
                }
                strcat(name8d3, ",");
                strcat(lfnstr, ",");

                snprintf(countstr, sizeof(countstr), "%d,", i++);
                if (n = extract_clustno(nextdirent))
                        snprintf(clusstr, sizeof(clusstr), "%d", n);
                else
                        snprintf(clusstr, sizeof(clusstr), "%s", "none");

                printf("%s %s %s %d, %s\n", \
                        countstr, name8d3, lfnstr, nextdirent->size, clusstr);
        }

        free(iterator);
}
