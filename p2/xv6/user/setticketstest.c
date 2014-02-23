#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"
#define check(exp, msg) if(exp) {} else {\
   printf(1, "%s:%d check (" #exp ") failed: %s\n", __FILE__, __LINE__, msg);\
   exit();}

/**
 * See if we can set the number of tickets for a process
 */
int
main(int argc, char *argv[])
{
  // @todo: add settickets test & test lottery as well... 
}
