#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"
#include "pstat.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

int
sys_getpinfo(void)
{
  char* placeHolder;
  argptr(0, &placeHolder, sizeof(struct pstat));

  struct pstat* st = (struct pstat*)(placeHolder);
  if (st) {
    *st = (currProcessInfo);
    return 0;
  }
  
  return -1;
}

/**
 * Sets the number of tickets of the calling process. By default each process
 * gets one ticket. By repeatedly calling this routine, a process can raise the
 * number of tickets it has to get a high proportion of CPU cycles.
 *
 * @return 0 on success, -1 on failure
 */
int
sys_settickets(void)
{
  int num;

  // Less than one ticket isn't possible.
  argint(0, &num);
  if(num < 1) return -1;
  
  // Give them as many tickets as they want! I don't see how this makes sense, 
  // but this seems to be the spec.
  proc->tickets = num;

  return 0;
}

/**
 * Xorshift algorithm random number generator. This is the JavaScript core of 
 * the same random number generator used in Webkit (chrome / safari) browsers.
 */
int
sys_lottery(void) 
{
  static int x = 373932311;
  static int y = 98716523;
  static int z = 123498473;
  static int w = 384837112;
  int t;
 
  t = x ^ (x << 11);
  x = y; y = z; z = w;
  return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}
