#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "user/user.h"

int main() {
  int pid;
  char c;

  pid = fork();
  if (pid == 0) {
    c = '/';
  } else {
    printf("parent pid = %d, child pid = %d\n", getpid(), pid);
    c = '\\';
  }
  sleep(2);
  for (int i = 0;; i++) {
    write(1, &c, 1);
  }
}