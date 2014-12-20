# fizanagi

## Goal
recover 1 cluster file from fat32

## Functionality
* show file system info +OK+
* list root directory 
 * sub directory +OK+
 * (8.3name) & info +OK+
 * (long file name) & info +OK+
 * root more than 1 cluster +<b>none</b>+
* file recovery (1 cluster at root) +<b>none</b>+
 * search +OK+
 * dump out +OK>
 * identify ambiguity +<b>none</b>+
  * long file name ambiguity+<b>none</b>+

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

## 感想
食屎啦BillGate$
