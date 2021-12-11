#include "kernel/types.h"
#include "kernel/param.h"
#include "user.h"
#define BUF_SIZE 512
int main(int argc, char **argv)
{
  if(argc < 2)
    exit(0);

  int cnt, i, pid;
  char buf[BUF_SIZE], *args[MAXARG+1], *p;
  p = buf;
  while(0 != (cnt = read(0, p, BUF_SIZE)))
  {
    if(p[cnt-1] != '\n')
    {
      p += cnt;
      continue;
    }
    p[cnt-1] = 0;
    if(p != buf)
      cnt = p - buf;
    else
      --cnt;
    buf[cnt] = 0;
    // printf("read %d bytes\n%s\n", cnt, buf);
    for(i = 0; i < argc; ++i)
      args[i] = argv[i+1];
    cnt = argc - 1;
    p = buf;
    while(*p != 0 && cnt < MAXARG)
    {
      args[cnt] = p;
      ++cnt;
      while(*p != ' ' && *p != 0)
        ++p;
      *p++ = 0;
    }
    args[cnt] = 0;
    // printf("input: %s\n", buf);
    // for(i = 0; i <= cnt; ++i)
    //   printf("%s\n", args[i]);

    pid = fork();
    if(pid == 0)
    {
      exec(args[0], args);
      printf("exec failed\n");
      exit(-1);
    }
    p = buf;
  }
  while(-1 != wait(0));
  exit(0);
}