#include <asm/types.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

#define READ_N 1024
#define DAMAGED_INDICATOR 0x0ffffff7  /* below this and above 0 is allocated fat entry*/

/*
 * The structure of bootsector copied from tutorial notes.
 * only 89 bytes are used, but the struct is 512 bytes.
 * It is copied from dosfsck source code.
 * Its the same as the one given by tutorial notes, but (much*100) clearer!
 */

struct boot_sector {
    __u8	ignored[3];	/* Boot strap short or near jump */
    __u8	system_id[8];	/* Name - can be used to special case
				   partition manager volumes */
    __u8	sector_size[2];	/* bytes per logical sector */
    __u8	cluster_size;	/* sectors/cluster */
    __u16	reserved;	/* reserved sectors */
    __u8	fats;		/* number of FATs */
    __u8	dir_entries[2];	/* root directory entries */
    __u8	sectors[2];	/* number of sectors */
    __u8	media;		/* media code (unused) */
    __u16	fat_length;	/* sectors/FAT */
    __u16	secs_track;	/* sectors per track */
    __u16	heads;		/* number of heads */
    __u32	hidden;		/* hidden sectors (unused) */
    __u32	total_sect;	/* number of sectors (if sectors == 0) */

    /* The following fields are only used by FAT32 */
    __u32	fat32_length;	/* sectors/FAT */
    __u16	flags;		/* bit 8: fat mirroring, low 4: active fat */
    __u8	version[2];	/* major, minor filesystem version */
    __u32	root_cluster;	/* first cluster in root directory */
    __u16	info_sector;	/* filesystem info sector */
    __u16	backup_boot;	/* backup boot sector */
    __u8 	reserved2[12];	/* Unused */

    __u8        drive_number;   /* Logical Drive Number */
    __u8        reserved3;      /* Unused */

    __u8        extended_sig;   /* Extended Signature (0x29) */
    __u32       serial;         /* Serial number */
    __u8        label[11];      /* FS label */
    __u8        fs_type[8];     /* FS Type */

    /* fill up to 512 bytes */
    __u8	junk[422];
} __attribute__ ((packed));

/*
 * The struct boot_sector is too clumsy.
 * Now extract the info we want from the file descriptor.
 * It is assumed the the fd is already at boot sector.
 */
int load_info(int fd, struct fat_info *fatfs)
{
        struct boot_sector bsec;
        if (sread(fd, &bsec, sizeof(bsec)) == -1)
                exit_perror(1, "read boot sector");

        fatfs->nfats = bsec.fats;

        /* noting that sector_size and sectors field is unaligned, it is safer to
         * byte-copy */
        unsigned short sec_size;
        unsigned short sectors_trial;  /* this is trial only */
        memcpy(&sec_size, &(bsec.sector_size), sizeof(unsigned short));
        fatfs->sector_size = sec_size;
        memcpy(&sectors_trial, &(bsec.sectors), sizeof(unsigned short));
        fatfs->sectors = (sectors_trial ? sectors_trial : bsec.total_sect);

        memcpy(&(fatfs->sectors), &(bsec.sectors), sizeof(unsigned short));

        fatfs->reserved_clusters = bsec.reserved;

        fatfs->fat_size = bsec.fat32_length * sec_size;
        fatfs->cluster_size = bsec.cluster_size * sec_size;

        fatfs->fat_location = bsec.reserved * sec_size;
        fatfs->cluster_start = bsec.reserved * sec_size + fatfs->nfats*fatfs->fat_size ;
        fatfs->root_cluster = bsec.root_cluster;

        fatfs->fd = fd;

        /* from dosfsck boot.c
        fs->root_entries = GET_UNALIGNED_W(b.dir_entries);
        fs->data_start = fs->root_start+ROUND_TO_MULTIPLE(fs->root_entries <<
                MSDOS_DIR_BITS,logical_sector_size);
        data_size = (off_t)total_sectors*logical_sector_size-fs->data_start;
        fs->clusters = data_size/fs->cluster_size;
        */

        /*
         * I have understand what it means after a 5hr of testing ......
         * Normally you may think total number of clusters is the same as
         * number of fat entries, BUT IT IS WRONG.
         * +++++++++++++++++++++++++++++++
         * Go To Hell FAT32 and Bill Gate$
         * +++++++++++++++++++++++++++++++
         * Some of the entries will never be used.  The REAL number of cluster
         * is the size of data area (area from root directoy) over cluster
         * size.
         */

        int data_size = fatfs->sectors*(fatfs->sector_size)-fatfs->cluster_start;
        fatfs->clusters = data_size/(fatfs->cluster_size);
}

/*
 * set fatfs->[clusters, freecluaters, allocatedclusters]
 * these are not needed for file recovery, and this function need heavy
 * computation.  So, it only called  with -i command line arg passed.
 */
void load_info_more(struct fat_info *fatfs)
{
        uint32_t buf[READ_N];  /* This will be an array storing fat entries */

        if ((fatfs->fat_size % ENTRY_SIZE) != 0)
                fprintf(stderr, "Fatsize not multiple of 32bits, may be corrupted.\n");

        /*
         * refer to dosfsck fat.c line 75 the effective number of entries is +2
         * don't askme why
         * OK, after googling, there is no cluster #0 and #1,
         * that means fat entry #0 #1 contain rubbish, and the 1st clustter is
         * indexed 1 in the fat.
         * still, dont ask why
         */
        int total_entries = fatfs->clusters + 2;
        DEBUG("sectors %d\n", fatfs->sectors);
        DEBUG("total cluster: %d\n", fatfs->clusters);
        unsigned int got;
        unsigned int freecl = 0;
        unsigned int allocated = 0;

        struct fat_iterstate *state = malloc(sizeof(struct fat_iterstate));
        init_iterfat(state, fatfs);
        int64_t fatent;

        while ((fatent = iterfat(state)) != -1) {
                if (fatent == 0)
                        freecl++;
                else if (fatent != DAMAGED_INDICATOR)
                        allocated++;
                else
                        DEBUG("strange at cluster, value %d\n", fatent);
        }

        free(state);
        fatfs->free_clusters = freecl;
        fatfs->allocated_clusters = allocated;
}
