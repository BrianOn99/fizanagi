#ifndef MYLFN_H
#define MYLFN_H

#define CHAR_PER_LFN ((10 + 12 + 4) / 2)

int extract_lfn(void *lfn, char *asciidest);

#endif
