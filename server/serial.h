#ifndef _SERIAL_H
#define _SERIAL_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
typedef struct env_data
{
	short int tmp;
	short int hum;
	int light;
}data_t;

int serial_open();
int serial_init();
void *serial_recv();
int serial_send();
int serial_fun();
int serial_close();
#endif
