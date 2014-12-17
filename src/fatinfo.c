#include <asm/types.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "common.h"

#define READ_N 1024
#define DEBUG 1

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
                exit_error(1, "read boot sector");

        fatfs->nfats = bsec.fats;

        /* noting that sector_size and sectors field is unaligned, it is safer to
         * byte-copy */
        unsigned int sec_size;
        memcpy(&sec_size, &(bsec.sector_size), sizeof(unsigned short));
        fatfs->sector_size = sec_size;
        memcpy(&(fatfs->sectors), &(bsec.sectors), sizeof(unsigned short));

        fatfs->reserved_clusters = bsec.reserved;

        fatfs->fat_size = bsec.fat32_length * sec_size;
        fatfs->cluster_size = bsec.cluster_size * sec_size;

        fatfs->fat_location = bsec.reserved * sec_size;
        fatfs->root_location = (bsec.reserved + fatfs->nfats*fatfs->fat_size) * sec_size;

        fatfs->fd = fd;

        //fatfs->free_clusters = count_free(fats);
}

void load_info_more(struct fat_info *fatfs)
{
        /* from dosfsck boot.c
        fs->root_entries = GET_UNALIGNED_W(b.dir_entries);
        fs->data_start = fs->root_start+ROUND_TO_MULTIPLE(fs->root_entries <<
                MSDOS_DIR_BITS,logical_sector_size);
        data_size = (off_t)total_sectors*logical_sector_size-fs->data_start;
        fs->clusters = data_size/fs->cluster_size;
        */

        int data_size = fatfs->sectors*fatfs->sector_size-fatfs->root_location;
        fatfs->clusters = data_size/fatfs->cluster_size;

        uint32_t buf[READ_N];  /* This will be an array storing fat entries */

        if ((fatfs->fat_size % ENTRY_SIZE) != 0)
                fprintf(stderr, "Fatsize not multiple of 32bits, may be corrupted.\n");
        
        int total_entries = fatfs->fat_size / ENTRY_SIZE;
#if DEBUG
        printf("DEBUG total cluster: %d\n", fatfs->clusters);
#endif
        int got;
        int free;
        for (int i=2; i < total_entries; i+=READ_N) {
                if ((total_entries - i) < READ_N)
                        got = get_fatentries(fatfs, &buf, i, total_entries - i);
                else
                        got = get_fatentries(fatfs, &buf, i, READ_N);

                for (int j=0; j < got; j++) {
                        if (buf[j] == 0)
                                free++;
#if DEBUG
                        else
                                printf("nonfree at %dth\n", i+j);
#endif
                }
        }

        fatfs->free_clusters = free;
}
