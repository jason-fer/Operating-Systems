#include "types.h"
#include "stat.h"
#include "user.h"
#include "pstat.h"
#include "param.h"

#define check(exp, msg) if(exp) {} else {\
   printf(1, "%s:%d check (" #exp ") failed: %s\n", __FILE__, __LINE__, msg);\
   exit();}

int
spin() {
   while (1) {
      int i, j;
      for (i = 0; i < 10000000; i++) {
         j = i % 11;
      }
   }
}

int
main(int argc, char *argv[])
{
   int pids[2];
   int ppid = getpid();
   int r, i, j;

   pids[0] = fork();
   if (pids[0] == 0) {
      r = settickets(2);
      if (r != 0) {
         printf(1, "settickets failed");
         kill(ppid);
      }
      spin();
   }

   pids[1] = fork();
   if (pids[1] == 0) {
      r = settickets(4);
      if (r != 0) {
         printf(1, "settickets failed");
         kill(ppid);
      }
      spin();
   }

   int count = 0;
   for (; count < 50; ++count) {
   
     sleep(1);

     int lticks[] = {-1, -1};
     int hticks[] = {-1, -1};
     struct pstat st;
     check(getpinfo(&st) == 0, "getpinfo");

     for(i = 0; i < NPROC; i++) {
       for(j = 0; j < 2; j++) {
         if (st.inuse[i] && st.pid[i] == pids[j]) {
           lticks[j] = st.lticks[i];
           hticks[j] = st.hticks[i];
         }
       }
     }

     int total_tickets = 0;
     int total_cpu_time = 0;
     for(i = 0; i < NPROC; ++i) {
       if (st.inuse[i]) {
         // total tickets
         total_tickets += st.tickets[i];

         // total cpu time
         total_cpu_time += st.hticks[i] + st.lticks[i];
       }
     }

     for(i = 0; i < NPROC; ++i) {
       if (st.inuse[i]) {
         printf(1, "Time: %d, pid: %d, cpu-time: %d total-cpu:%d tickets: %d total-tickets: %d queue: %s \n" , 
            count, st.pid[i], st.hticks[i]+st.lticks[i], total_cpu_time, st.tickets[i], total_tickets, st.mlfq[i]?"low":"high" );
       }
     }
   }

   /* for (i = 0; i < 2; i++) { */
   /*    kill(pids[i]); */
   /*    wait(); */
   /* } */

   /* for (i = 0; i < 2; i++) { */
   /*    check(hticks[i] > 0 && lticks[i] > 0, "each process should have some hticks and lticks"); */
   /* } */

   /* float ratio = (float)lticks[0] / (float)lticks[1]; */
   /* printf(1, "lticks child 1: %d\n", lticks[0]); */
   /* printf(1, "lticks child 2: %d\n", lticks[1]); */
   /* check(ratio > .25 && ratio < .75, "ratio should be about 1/2"); */

   /* printf(1, "TEST PASSED\n"); */
   exit();
}
