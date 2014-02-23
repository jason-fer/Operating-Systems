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
  if(argint(0, &num) < 1) return -1;
  
  proc->tickets = num;

  return 0;

}