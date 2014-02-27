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
   int pids[3];
   int ppid = getpid();
   int r, i, j;

   pids[0] = fork();
   if (pids[0] == 0) {
      r = settickets(10);
      if (r != 0) {
         printf(1, "settickets failed");
         kill(ppid);
      }
      spin();
   }

   pids[1] = fork();
   if (pids[1] == 0) {
      r = settickets(30);
      if (r != 0) {
         printf(1, "settickets failed");
         kill(ppid);
      }
      spin();
   }

   pids[2] = fork();
   if (pids[2] == 0) {
      r = settickets(57);
      if (r != 0) {
         printf(1, "settickets failed");
         kill(ppid);
      }
      spin();
   }

   int count = 0;
   int incr = 10;

   // Header row:
   printf(1, "Time,Pid,cpu-time,total-cpu,tickets,total-tickets,queue\n");

   for (; count <= 100; count = count + incr) {
   
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
         // printf(1, "Time: %d, pid: %d, cpu-time: %d total-cpu:%d tickets: %d total-tickets: %d queue: %s \n" , 
         //    count, st.pid[i], st.hticks[i]+st.lticks[i], total_cpu_time, st.tickets[i], total_tickets, st.mlfq[i]?"low":"high" );

         printf(1, "%d,%d,%d,%d,%d,%d,%s\n" , 
            count, st.pid[i], st.hticks[i]+st.lticks[i], total_cpu_time, st.tickets[i], total_tickets, st.mlfq[i]?"low":"high" );
       }
     }

     sleep(incr);

   }

   exit();
}
