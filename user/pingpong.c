#include "kernel/types.h"
#include "user.h"
int main(int argc, char **argv)
{
  int ping[2], pong[2], pid, cnt;
  char bytes[2] = {'p', 'c'};
  char buf[2];
  pipe(ping);
  pipe(pong);
  pid = fork();
  if(pid == 0)
  {
    close(ping[1]);
    close(pong[0]);
    cnt = read(ping[0], buf, 2);
    if(cnt > 0)
      printf("%d: received ping\n", getpid());
    write(pong[1], bytes+1, 1);
  }
  else
  {
    close(ping[0]);
    close(pong[1]);
    write(ping[1], bytes, 1);
    cnt = read(pong[0], buf, 2);
    if(cnt > 0)
      printf("%d: received pong\n", getpid());
  }
  exit(0);
}