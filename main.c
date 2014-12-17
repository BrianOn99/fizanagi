#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"
#include "fatinfo.h"
#define DEBUG 0

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


enum {info, list, recover, long_recover} action;
char *target = NULL;
char *dest = NULL;
char *device = NULL;

int main(int argc, char **argv)
{
        int fd;
        parse_options(argc, argv);

#if DEBUG
        printf("%d\n", action);
        if (target)
                printf("target: %s\n", target);
        if (dest)
                printf("dest: %s\n", dest);

        printf("device: %s\n", device);
#endif
        if ((fd = open(device, O_RDONLY)) == -1)
                exit_error(1, "open");
        
        struct fat_info fatfs;
        load_info(fd, &fatfs);
        load_info_more(&fatfs);

        print_info(&fatfs);
}

void print_info(struct fat_info *fatfs)
{
        struct tuple { char *desc; int value; } tups[] = {
                {"FATS", fatfs->nfats},
                {"bytes per sector", fatfs->sector_size},
                {"sectors per cluster", fatfs->cluster_size / fatfs->sector_size},
                {"reserved sectors", fatfs->reserved_clusters},
                {"DEBUG fatsize", fatfs->fat_size},
                /* not implemented yet
                {"allocated clusters", XXX},
                */
                {"free clusters", fatfs->free_clusters},
        };

        for (int i=0; i < sizeof(tups)/sizeof(tups[0]); i++)
                printf("Number of %s = %d\n", tups[i].desc, tups[i].value);
}

int parse_options(int argc, char **argv)
{
        int c;
        if ((c = getopt(argc, argv, "d:")) == -1)
                usage_exit();
        else 
                device = optarg;

        if ((c = getopt(argc, argv, "ilr:R:")) == -1)
                usage_exit();
        switch (c) {
                case 'i':
                        action = info;
                        break;
                case 'l':
                        action = list;
                        break;

                case 'r':
                case 'R':
                        action = c == 'r' ? recover : long_recover;
                        target = optarg;

                        if ((c = getopt(argc, argv, "o:")) == -1)
                                usage_exit();
                        switch (c) {
                                case 'o':
                                        dest = optarg;
                                        break;
                                case '?':
                                        usage_exit();
                        }
                        break;

                case '?':
                        usage_exit();
                default:
                        /* Never reach here */
                        abort();
        }

        if (optind != argc)
                /* not all arguments parsed */
                usage_exit();
}
