#ifndef FATINFO_H
#define FATINFO_H

#include "common.h"

int load_info(int fd, struct fat_info *fatfs);
void load_info_more(struct fat_info *fatfs);

#endif
