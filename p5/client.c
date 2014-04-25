#include <stdio.h>
#include <sys/select.h>
#include "udp.h"
#include "mfs.h"

int
main(int argc, char *argv[])
{
	int response = MFS_Init("localhost", 10000);
  assert(response >= 0);
}