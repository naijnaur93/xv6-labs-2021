#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  if (argc != 1) {
    fprintf(2, "ping pong has no parameters!");
    exit(1);
  }

  int p[2];

  const int ping = 10;
  const int pong = 20;
  int buf;

  pipe(p);
  if (fork() == 0) {
    read(p[0], &buf, 4);
    if (buf == ping) {
      // received ping, respond pong
      fprintf(1, "%d: received ping\n", getpid());
      write(p[1], &pong, 4);
      exit(0);
    } else {
      fprintf(2, "error receiving ping");
      exit(1);
    }
  } else {
    write(p[1], &ping, 4);
    read(p[0], &buf, 4);
    if (buf == pong) {
      fprintf(1, "%d: received pong\n", getpid());
      exit(0);
    } else {
      fprintf(2, "error receiving pong");
      exit(1);
    }
  }
}