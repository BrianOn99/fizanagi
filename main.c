#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#define DEBUG

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

int main(int argc, char **argv)
{
        enum {info, list, recover, long_recover} action;
        char *target = NULL;
        char *dest = NULL;
        int c;

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

#ifdef DEBUG
        printf("%d\n", action);
        if (target)
                printf("target: %s\n", target);
        if (dest)
                printf("dest: %s\n", dest);
#endif
}

