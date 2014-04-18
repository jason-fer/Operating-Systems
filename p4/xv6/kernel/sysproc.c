#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "sysfunc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_clone(void)
{
  char* placeHolder;
  void(*fcn)(void*);
  int size = sizeof(fcn);
  argptr(0, &placeHolder, size);

  fcn = (void (*) (void *))(placeHolder);

  argptr(1, &placeHolder, sizeof(void*));
  void* arg = (void*)placeHolder;

  if (argptr(2, &placeHolder, PGSIZE) < 0)
    return -1;

  void* stack = (void*)placeHolder;

  return clone(fcn, arg, stack);
}

int
sys_join(void)
{
  int placeHolder;
  void** stack;

  if (argint(0, &placeHolder) < 0)
    return -1;

  stack = (void**)(placeHolder);
  /* int* addr = (uint*)stack; */
  int stackVal;

  if (fetchint(proc, placeHolder, &stackVal) < 0)
    return -1;

  if (stackVal%PGSIZE != 0)
    return -1;

  return join(stack);
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
sys_wakeup_thread(void)
{
  char* placeHolder;
  if (argptr(0, &placeHolder, sizeof(char*)) < 0) 
    return -1;
 
  cond_t* cv = (cond_t*)(placeHolder);
  wakeup_thread(cv);
  return 0;
}

int
sys_sleep_thread(void)
{
  char* placeHolder;
  if (argptr(0, &placeHolder, sizeof(char*)) < 0) 
    return -1;

  cond_t* cv = (cond_t*)(placeHolder);
  sleep_thread(cv);
  return 0;
}
