#ifndef READCL_H
#define READCL_H

#include <asm/types.h>
#include <stdint.h>
#include <stdbool.h>

struct dirent {
    __u8	name[8],ext[3];	/* name and extension */
    __u8	attr;		/* attribute bits */
    __u8	lcase;		/* Case for base and extension */
    __u8	ctime_ms;	/* Creation time, milliseconds */
    __u16	ctime;		/* Creation time */
    __u16	cdate;		/* Creation date */
    __u16	adate;		/* Last access date */
    __u16	starthi;	/* High 16 bits of cluster in FAT32 */
    __u16	time,date,start;/* time, date and first cluster */
    __u32	size;		/* file size (in bytes) */
};

struct iterstate {
        ssize_t mysize;
        uint32_t cluster_i;
        unsigned int dir_i; /* current index in dir[] , internal use */
        struct fat_info *fatfs;
        int count;
        bool allow_deleted;
        /* zero length array, should be (same size as cluster) * 2, C99 extension
         * this will save all dirent in the cluster */
        struct dirent dir[];
};

enum dirent_type { LFN, DIRECTORY, NORMALFILE, NOTCARE };
enum nametype { LN, SN };

struct iterstate *init_iter(struct fat_info *fatfs,
                            unsigned int cluster_i, bool allow_deleted);
struct dirent *iterdirent(struct iterstate *state, char *name);
void readcluster(struct fat_info *fatfs, void *buf, unsigned int index);
void lsdir(struct fat_info *fatfs, unsigned int cluster_i);
void find_n_recover(struct fat_info *fatfs, unsigned int cluster_i,
                          char *find_name, char *out_name, enum nametype type);

#endif
