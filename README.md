[toc]



# FOREWORD

This is a **portal** for my project of [MIT 6.S081(Operating System), 2021 Fall](https://pdos.csail.mit.edu/6.S081/2021/schedule.html). 

You can get a glance at what I've done in this project. To further inspect/test my code, please switch branches and see instructions below.

# Test Instructions

The test can ONLY be done one by one. 

To inspect my code, first switch to the branch you wish. For example, if you want to see my implementation of `Lab Util`, first type:

```
$ git checkout util
```

Then you are free to browse my code for `Lab Util`.

To test it, run:

```
$ make grade
```

It will automatically run all test cases and finally give out my scores.

# What I've Done

*(subject to update because it's currently under progress)*

**Progress: 2 / 10**

## Lab 1 - Unix Utilities

The original requirements can be found [here](https://pdos.csail.mit.edu/6.S081/2021/labs/util.html).

### Tasks

To implement following functions:

- `sleep`: Pause for a user-specified number of ticks
- `pingpong`: Use UNIX system calls to "ping-pong" a byte between two processes over a pair of pipes, one for each direction. 
- `primes`: Write a concurrent version of prime sieve using pipes.
- `find`: Write a simple version of the UNIX find program: find all the files in a directory tree with a specific name.
- `xargs`: Write a simple version of the UNIX xargs program: read lines from the standard input and run a command for each line, supplying the line as arguments to the command.

### Relevant Files

**REMINDER: PLEASE SWITCH TO THE CORRECT BRANCH BY: `$ git checkout util`**

```
user/sleep.c
user/pingpong.c
user/primes.c
user/find.c
user/xargs.c
```

## Lab 2 - System Calls

The original requirements can be found [here](https://pdos.csail.mit.edu/6.S081/2021/labs/syscall.html).

### Tasks

To implement the following two functions in the kernel:

- `trace`: Add a system call tracing feature. It should take one argument, an integer "mask", whose bits specify which system calls to trace. Modify the xv6 kernel to print out a line when each system call is about to return, if the system call's number is set in the mask.
- `sysinfo`: It collects information about the running system. The system call takes one argument: a pointer to a `struct sysinfo` (see `kernel/sysinfo.h`). The kernel should fill out the fields of this struct: the `freemem` field should be set to the number of bytes of free memory, and the `nproc` field should be set to the number of processes whose `state` is not `UNUSED`.

### Relevant Files

**REMINDER: PLEASE SWITCH TO THE CORRECT BRANCH BY: `$ git checkout syscall`**

```
kernel/proc.c
kernel/syscall.c
kernel/kalloc.c
```

## Lab 3 - Page Table

The original requirements can be found [here](https://pdos.csail.mit.edu/6.S081/2021/labs/pgtbl.html).

### Tasks

To implement the following three functions in the kernel:

- **Make it possible for processes in user mode to directly retrieve `pid` without entering kernel.** When each process is created, map one read-only page at USYSCALL (a VA defined in memlayout.h). At the start of this page, store a struct usyscall (also defined in memlayout.h), and initialize it to store the PID of the current process. 
- `vmprint()`: It should take a pagetable_t argument, and print that pagetable.
- `sys_pgaccess()`: return the info of accessed pages to the buffer passed in by user.

### Relevant Files and Test

**See the report in the branch `pgtbl` by:**

```
$ git checkout pgtbl
```

# Test Results

## Lab 1

![image-20211010200710258](README.assets/image-20211010200710258.png)

### Lab 2

![image-20211019210027385](README.assets/image-20211019210027385.png)

