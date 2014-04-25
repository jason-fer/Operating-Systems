#include <stdio.h>
#include <sys/select.h>
#include "udp.h"
#include "mfs.h"

int
main(int argc, char *argv[])
{
	MFS_Init("localhost", 10000);
}