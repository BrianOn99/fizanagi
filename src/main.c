#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"
#include "fatinfo.h"
#include "readcluster.h"

int parse_options(int argc, char **argv);
void print_info(struct fat_info *fatfs);

void usage_exit(void)
{
        fputs(
               "Usage: ./recover -d [device filename] [other arguments]\n"
               "-i                        Print boot sector information\n"
               "-l                        List all the directory entries\n"
               "-r target -o dest         File recovery with 8.3 filename\n"
               "-R target -o dest         File recovery with long filename\n",
               stdout);

        exit(EXIT_FAILURE);
}


enum act {info = 1, list, recover, long_recover} action;
char *target = NULL;
char *dest = NULL;
char *device = NULL;

int main(int argc, char **argv)
{
        int fd;
        parse_options(argc, argv);

        DEBUG("action: %d\n", action);
        DEBUG("device: %s\n", device);
        if (target)
                DEBUG("target: %s\n", target);
        if (dest)
                DEBUG("dest: %s\n", dest);

        if ((fd = open(device, O_RDONLY)) == -1)
                exit_perror(1, "open");
        
        struct fat_info fatfs;
        load_info(fd, &fatfs);

        if (action == info) {
                load_info_more(&fatfs);
                print_info(&fatfs);
        } else if (action == list) {
                lsdir(&fatfs, fatfs.root_cluster);
        } else if (action == recover || action == long_recover) {
                DEBUG("action %d, 8d3 %d\n", action, is8d3(target));
                enum nametype type = (action == long_recover) ? LN : SN;
                find_n_recover(&fatfs, fatfs.root_cluster, target, dest, type);
        }
}

void print_info(struct fat_info *fatfs)
{
        struct tuple { char *desc; int value; } tups[] = {
                {"FATs", fatfs->nfats},
                {"bytes per sector", fatfs->sector_size},
                {"sectors per cluster", fatfs->cluster_size / fatfs->sector_size},
                {"reserved sectors", fatfs->reserved_clusters},
                {"allocated clusters", fatfs->allocated_clusters},
                {"free clusters", fatfs->free_clusters},
#if _DEBUG
                {"DEBUG fat start", fatfs->fat_location},
                {"DEBUG fatsize", fatfs->fat_size},
                {"DEBUG sectors", fatfs->sectors},
                {"DEBUG cluster start", fatfs->cluster_start},
                {"DEBUG root cluster start", fatfs->cluster_start + \
                        fatfs->cluster_size * (fatfs->root_cluster - 2)},
#endif
        };

        for (int i=0; i < sizeof(tups)/sizeof(tups[0]); i++)
                printf("Number of %s = %d\n", tups[i].desc, tups[i].value);
}

void setaction(enum act i)
{
        if (action)
                usage_exit();
        action = i;
}

int parse_options(int argc, char **argv)
{
        int c;
        while ((c = getopt(argc, argv, "ild:r:R:o:")) != -1) {
                switch (c) {
                        case 'd':
                                if (device)
                                        usage_exit();
                                device = optarg;
                                break;
                        case 'i':
                                setaction(info);
                                break;
                        case 'l':
                                setaction(list);
                                break;
                        case 'o':
                                if (dest)
                                        usage_exit();
                                dest = optarg;
                                break;

                        case 'r':
                        case 'R':
                                setaction(c == 'r' ? recover : long_recover);
                                target = optarg;
                                break;

                        case '?':
                                usage_exit();
                        default:
                                /* Never reach here */
                                abort();
                }
        }

        if (optind != argc || !device || !action || \
            ((action == recover) && !dest)) {
                usage_exit();
        }
}
