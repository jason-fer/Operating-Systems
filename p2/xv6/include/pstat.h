#ifndef _PSTAT_H_
#define _PSTAT_H_

#include "param.h"

struct pstat {
  // whether this slot of the process process table is in use (1 or 0)
  int inuse[NPROC]; 

  // the PID of each process
  int pid[NPROC];

  // the number of ticks each process has accumulated at HIGH priority
  int hticks[NPROC];

  // the number of ticks each process has accumulated at LOW priority
  int lticks[NPROC];
  
  // MLFQ priority: 0 = HIGH, 1 = LOW (based on proc.h mlfq enum)
  int mlfq[NPROC];

  // The number of lottery tickets each process currently has.
  int tickets[NPROC];
};

struct pstat currProcessInfo;


#endif // _PSTAT_H_
