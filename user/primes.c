/**
 * This function implements its execution using the method like "vectorized model", which
 * is mentioned in CMU's 15-445: Query Execution I. 
 * 
 * But it can also be implemented by "volcano model". Try it later
*/

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void primes(int pipeline[2]) {
  //fprintf(1, "prime under process %d\n", getpid());
  int buf[20];    // buffer got from its parent
  int buf1[20];   // buffer to send to its child
  int prime;
  int size;       // size of the buffer got from its parent
  if ((size = read(pipeline[0], &buf, 80)) != 0) {
    // remember the prime, which is first number got from the left pipeline
    prime = buf[0];
    size /= 4;    // a int is 4 bytes, so size should be one fourth
    fprintf(1, "prime %d\n", prime, size);
  } else {
    exit(0);
  }

  int p[2];
  pipe(p);  // create pipeline for its child process
  if (fork() == 0) {
    close(pipeline[0]);
    close(p[1]);
    primes(p);
    exit(0);
  } else {
    // read from the left pipeline until it exhausts
    int i;
    int cnt = 0;
    for (i = 1; i < size; i++) {
      if (buf[i] % prime != 0) {
        // filter numbers which can be divided by prime
        buf1[cnt++] = buf[i];
      }
    }
    // write all numbers which cannot be devided by prime to the right pipeline
    write(p[1], buf1, 4*cnt);
    // wait for a little to let the child process to read the write
    sleep(1);
    close(p[0]);
    close(p[1]);
    wait(0);
  }
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc != 1) {
    fprintf(2, "error: primes does not need parameters!");
    exit(1);
  }

  int i = 2;
  int prime = i;
  fprintf(1, "prime %d\n", i++);

  // prepare buffer for its child process
  int buf[20], cnt = 0;
  for (; i <= 35; i++) {
    if (i % prime != 0) {
      buf[cnt++] = i;
    }
  }

  int p[2];
  pipe(p);
  if (fork() == 0) {
    primes(p);
    exit(0);
  } else {
    write(p[1], buf, 4*cnt);
    close(p[0]);
    close(p[1]);
    wait(0);
  }
  exit(0);
}