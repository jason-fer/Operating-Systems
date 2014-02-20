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
  struct pstat* st = NULL;
  argpstat(0, st);
  if (st) {
    st = &(currProcessInfo);
    return 0;
  }
  
  return -1;
}

/* int** */
/* getprocinfo(struct pstat* procInfo) { */
/*   int i = 0; */
/*   int count = 0; */
/*   int** info = NULL; */
/*   // Structure is going to be (hticks + lticks), pid and onHQ */
/*   for (; i < NPROC; ++i) { */
/*     if (procInfo->inuse[i]) { */
/*       count++; */
/*     } */
/*   } */
  
/*   if (count > 0) { */
/*     info = (int**) malloc(count*sizeof(int*)); */
/*     for (i = 0; i < count; ++i) { */
/*       info[i] = (int*)malloc(3*sizeof(int)); */
/*     } */

/*     for (i = 0; i < NPROC; ++i) { */
/*       if (procInfo->inuse[i]) { */
/*         info[i] = {(procInfo->hticks[i] + procInfo->lticks[i]), */
/*                    procInfo->pid[i], */
/*                    procInfo->onHQ[i]}; */
/*       } */
/*     } */
/*   } */
/* } */
