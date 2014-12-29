# fizanagi

## Goal
recover 1 cluster file from fat32

## Compile
give sane output:
```sh
make
```
if you want a dumb program that comform to csci3150 submission guideline:
```sh
make dumb
```
give debug message:
```sh
make debug
```

## Functionality
* show file system info +OK+
* list root directory 
 * sub directory +OK+
 * (8.3name) & info +OK+
 * (long file name) & info +OK+
 * root more than 1 cluster +OK+
* file recovery (1 cluster at root)
 * search +OK+
 * dump out +OK+
 * identify ambiguity +OK+
  * long file name ambiguity
* *extra*
 * recover multiple file +OK+
 * the -r and -R has no difference: the program auto search for all 8.3name and
   lfn +OK+
 * list deleted file +OK+
   (what is the use if you even dont know the name of deleted file?) 

## Handy command
```sh
dd if=/dev/zero of=fatsample bs=1M count=128
mkfs.vfat -s 2 -F 32 fatsample
sudo losetup /dev/loop0 fatsample
sudo mount /dev/loop0 ./mnt
sync  # flush to fatsample without umount
```

## Reference
Some algorithm and structure from
[dosfsck](http://daniel-baumann.ch/software/dosfstools/) 

Thus it is GPL3

## Coders
Chiu Yue Chun
He ZiXuan

## 感想
食屎啦BillGate$
