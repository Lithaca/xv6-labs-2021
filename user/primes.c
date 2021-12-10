#include "kernel/types.h"
#include "user.h"
////////////////////////////////////////////////////////////////////////////////
void process(int left, int right, int p)
{
  int n;
  while(0 != read(left, &n, sizeof(n)))
    if((n % p) != 0)
      write(right, &n, sizeof(n));
  close(left);
  close(right);
}
void generate(int left)
{
  int p, pid, rp[2];
  if(read(left, &p, sizeof(p)))
  {
    printf("prime %d\n", p);
    pipe(rp);
    pid = fork();
    if(pid == 0)
    {
      close(left);
      close(rp[1]);
      generate(rp[0]);
    }
    else
    {
      close(rp[0]);
      process(left, rp[1], p);
      wait(0);
    }
  }
  else
    close(left);
  exit(0);
}
int main(int argc, char *argv[])
{
  int i, pid, start = 2, end = 36;
  int lp[2];
  pipe(lp);
  pid = fork();
  if(pid == 0)
  {
    close(lp[1]);
    generate(lp[0]);
  }
  else
  {
    close(lp[0]);
    // Feed 2-35 to generated process
    for(i = start; i < end; ++i)
      write(lp[1], &i, sizeof(i));
    close(lp[1]);
    wait(0);
  }
  exit(0);
}