#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

char *readline() {
  static char buf[512];
  char c;
  int i = 0;
  while (read(0, &c, 1)) {
    if (c == '\n') {
      buf[i] = 0;
      return buf;
    }
    buf[i++] = c;
  }
  return 0;
}

char **FormArgvs(int old_argc, char *old_argv[], char *line, int free_flag) {
  static char *buf[MAXARG+1];
  int i;

  if (free_flag) {  // destruct buf
    for (i = old_argc - 1; i <= MAXARG; i++) {
      if (buf[i] != 0) {
        free(buf[i]);
      }
    }
    exit(0);
  }

  // copy original params
  for (i = 1; i < old_argc; i++) {
    buf[i - 1] = old_argv[i];
  }
  i--;
  // parse params from line, append them to the end of buf
  if (buf[i] == 0)
    buf[i] = malloc(40 * sizeof(char));
  char *p = line;
  int j = 0;
  for (; *p != 0; p++) {
    buf[i][j++] = *p;
    if (*p == ' ') {
      buf[i][j - 1] = 0;
      j = 0;
      i++;
      if (buf[i] == 0)
        buf[i] = malloc(40 * sizeof(char));
    }
    // check space availablility
    if (i >= MAXARG) {
      fprintf(2, "warning: too many argvs! Truncate...\n");
      buf[i] = 0;
      return buf;
    }
    if (j > 38) {
      fprintf(2, "warning: argv too long! Truncate...\n");
      buf[i][j] = 0;
      j = 0;
      buf[++i] = 0;
      return buf;
    }
  }
  // put end flag
  buf[i][j] = 0;
  return buf;
}

int main(int argc, char *argv[]) {
  char *line = readline();
  while (line != 0) {
    if (fork() == 0)
    {
      exec(argv[1], FormArgvs(argc, argv, line, 0));
      fprintf(2, "error: exec command\n");
      exit(1);
    }
    else
    {
      wait(0);
      line = readline();
    }
  }
  // before exit, free space
  FormArgvs(argc, argv, line, 1);
  exit(0);
}