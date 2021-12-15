# Bugs in the original code

## Description

After running `bcachetest`, several tests in `usertests`, such as `writebig`, `bigdir`, etc, will fail. 

To pass them, the file system should be cleaned by `$ make clean`.

## Reappearance

In command line, type:

```
(in host)$ make qemu
...a lot of output...
xv6 kernel is booting

hart 1 starting
hart 2 starting
init: starting sh
$ bcachetest
start test0
test0 results:
--- lock kmem/bcache stats
lock: kmem: #test-and-set 0 #acquire() 33036
lock: bcache: #test-and-set 29750 #acquire() 65930
--- top 5 contended locks:
lock: virtio_disk: #test-and-set 136647 #acquire() 1186
lock: proc: #test-and-set 38021 #acquire() 622334
lock: proc: #test-and-set 35869 #acquire() 604117
lock: proc: #test-and-set 33544 #acquire() 624644
lock: proc: #test-and-set 33493 #acquire() 604121
tot= 29750
test0: FAIL
start test1
test1 OK
$ usertests writebig
usertests starting
test writebig: panic: balloc: out of blocks

```

