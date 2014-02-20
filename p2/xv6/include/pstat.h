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
  
  // tests whether it is on HIGH priority or not
  int onHQ[NPROC];
};

struct pstat currProcessInfo;


#endif // _PSTAT_H_
