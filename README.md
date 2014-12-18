# fizangi

## Goal
recover 1 cluster file from fat32

## Functionality
* show file system info +OK+
* list root directory 
 * sub directory +OK+
 * (8.3name) & info +OK+
 * (long file name) & info +<b>identify only</b>+
 * root more than 1 cluster +<b>none</b>
* file recovery +<b>none</b>+
 * search +<b>OK (not yet test bug)</b>
 * dump out +<b>none</b>
 * identify ambiguity +<b>none</b>+
 * long file name +<b>none</b>+

## Handy command
```sh
sudo dd if=/dev/zero of=fatsample bs=1M count=128
sudo mkfs.vfat -s 2 -F 32 fatsample
sudo losetup /dev/loop0 fatsample
sudo mount /dev/loop0 ./mnt
```
## Reference
Some algorith and structure from
[dosfsck](http://daniel-baumann.ch/software/dosfstools/) 
Thus it is GPL3

## 感想
食屎啦BillGate$
