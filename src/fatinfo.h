#ifndef FATINFO_H
#define FATINFO_H

#include "common.h"

#define DAMAGED_INDICATOR 0x0ffffff7  /* below this and above 0 is allocated fat entry*/
#define EOF_INDICATOR 0x0ffffff8  /* below this and above 0 is allocated fat entry*/

int load_info(int fd, struct fat_info *fatfs);
void load_info_more(struct fat_info *fatfs);

#endif
