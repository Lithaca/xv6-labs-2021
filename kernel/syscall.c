#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"
#include "sysinfo.h"

// Fetch the uint64 at addr from the current process.
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if(addr >= p->sz || addr+sizeof(uint64) > p->sz)
    return -1;
  if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  int err = copyinstr(p->pagetable, buf, addr, max);
  if(err < 0)
    return err;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
int
argint(int n, int *ip)
{
  *ip = argraw(n);
  return 0;
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
int
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
  return 0;
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  if(argaddr(n, &addr) < 0)
    return -1;
  return fetchstr(addr, buf, max);
}

extern uint64 sys_chdir(void);
extern uint64 sys_close(void);
extern uint64 sys_dup(void);
extern uint64 sys_exec(void);
extern uint64 sys_exit(void);
extern uint64 sys_fork(void);
extern uint64 sys_fstat(void);
extern uint64 sys_getpid(void);
extern uint64 sys_kill(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_mknod(void);
extern uint64 sys_open(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_unlink(void);
extern uint64 sys_wait(void);
extern uint64 sys_write(void);
extern uint64 sys_uptime(void);
uint64 sys_trace(void);
extern uint64 sys_sysinfo(void);

static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_trace]   sys_trace,
[SYS_sysinfo] sys_sysinfo,
};

static char*syscall_name[] = {
  "",
  "fork",
  "exit",
  "wait",
  "pipe",
  "read",
  "kill",
  "exec",
  "fstat",
  "chdir",
  "dup",
  "getpid",
  "sbrk",
  "sleep",
  "uptime",
  "open",
  "write",
  "mknod",
  "unlink",
  "link",
  "mkdir",
  "close",
  "trace",
  "sysinfo",
};

void tracearg(int syscall_num);

void
syscall(void)
{
  int num;
  uint64 ret;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    ret = syscalls[num]();
    if(p->tracemask & (1 << num)) {
      printf("%d: syscall %s", p->pid, syscall_name[num]);

      // Print syscall arguments
      printf("(");
      tracearg(num);
      printf(")");

      if(num == SYS_exit)
        printf("\n");
    }
    if(p->tracemask & (1 << num)) {
      printf(" -> %d\n", ret);
    }
    p->trapframe->a0 = ret;
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}


uint64 sys_trace()
{
  uint64 mask = argraw(0);
  struct proc *p = myproc();
  p->tracemask = mask;
  return 0;
}

uint64 sys_sysinfo(void)
{
  uint64 addr;
  struct sysinfo info;
  struct proc *mp = myproc();
  if(argaddr(0, &addr) < 0)
    return -1;

  info.freemem = kfreemem();
  info.nproc = proc_amount();

  if(copyout(mp->pagetable, addr, (char*)&info, sizeof(info)) < 0)
    return -1;
  return 0;
}

void tracearg(int num)
{

  int argi = 0;
  uint64 argp = 0, *strlst;
  char args[128];

  switch(num) // arg1
  {
    // int
    case SYS_exit:
    case SYS_write:
    case SYS_read:
    case SYS_close:
    case SYS_kill:
    case SYS_fstat:
    case SYS_dup:
    case SYS_sbrk:
    case SYS_sleep:
    case SYS_trace:
    if(argint(0, &argi) < 0)
      return;
    printf("%d", argi);
    break;
    // int *, void *
    case SYS_wait:
    case SYS_pipe:
    case SYS_sysinfo:
    if(argaddr(0, &argp) < 0)
      return;
    printf("%p", argp);
    break;
    // char *
    case SYS_exec:
    case SYS_open:
    case SYS_mknod:
    case SYS_unlink:
    case SYS_link:
    case SYS_mkdir:
    case SYS_chdir:
    memset(args, 0, 128);
    if(argstr(0, args, 128) < 0)
      return;
    printf("\"%s\"", args);
    break;
    default:
    break;
  }

  switch(num) // arg2
  {
    // int
    case SYS_open:
    case SYS_mknod:
    if(argint(1, &argi) < 0)
      return;
    printf(", %d", argi);
    break;
    // void *
    case SYS_write:
    case SYS_read:
    case SYS_fstat:
    if(argaddr(1, &argp) < 0)
      return;
    printf(", %p", argp);
    break;
    // char *
    case SYS_link:
    memset(args, 0, 128);
    if(argstr(1, args, 128) < 0)
      return;
    printf(", \"%s\"", args);
    break;
    // char **
    case SYS_exec:
    if(argaddr(1, &argp) < 0)
      return;
    strlst = (uint64* )argp;
    for(argi = 0; fetchaddr((uint64)(strlst + argi), &argp) >= 0; ++argi)
    {
      if(argp == 0)
        break;
      memset(args, 0, 128);
      if(fetchstr(argp, args, 128) < 0)
        break;
      printf(", \"%s\"", args);
    }
    break;
    default:
    break;
  }

  switch(num) // arg3
  {
    // int
    case SYS_write:
    case SYS_read:
    case SYS_mknod:
    if(argint(2, &argi) < 0)
      return;
    printf(", %d", argi);
    break;
    default:
    break;
  }
}