#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>

#define ENTRY_SIZE 4  /* 32bits is 4 bytes */

struct fat_info {
        int nfats;
        unsigned int free_clusters;
        unsigned int allocated_clusters;
        unsigned int reserved_clusters;
        size_t sector_size;  /* size of one sector in byte, same below */ 
        size_t cluster_size;

        /* following are for internal use only */
        size_t fat_size;
        unsigned int sectors;
        unsigned int clusters;
        unsigned int fat_location;
        unsigned int cluster_start;
        unsigned int root_cluster;

        int fd;
};

int get_fatentries(struct fat_info *fatfs, void *pbuf, off_t index, int count);
int sread(int fd, void *buf, size_t size);
int spread(int fd, void *buf, size_t size, off_t offset);
void exit_error(int s, char *msg);
void exit_perror(int s, char *msg);

#endif
