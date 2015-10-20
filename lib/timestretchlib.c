/**                       Copyright (C) 2014 HPDCS Group
*                       http://www.dis.uniroma1.it/~hpdcs
* 
* This is free software; 
* You can redistribute it and/or modify this file under the
* terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version.
* 
* This file is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
* A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License along with
* this file; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
* 
* @file timestretchlib.c 
* @brief This is the library file which allows any process to easily rely on 
* 	 the facilities provided by the timestretch module
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @date November 11, 2014
*/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>

#include <module/timestretch.h>
#include "timestretch.h"



static int fd = -1;
static int ret = 0;


//flags buff[TS_THREADS] = {[0 ... TS_THREADS-1] {0,0,0}};

static __thread int pid;
static __thread map lookup = {-1,-1};

int ts_open(void) {

  int ret;

  if (fd >= 0){
	return TS_ALREADY_OPEN;
  }

  fd = open("/dev/timestretch", O_RDONLY);

  if (fd == -1) {
	return TS_OPEN_ERROR;
  }

//  ret = ioctl(fd, IOCTL_SETUP_BUFFER,buff);
//  if (ret == -1) {
//	close(fd);
//	return TS_OPEN_ERROR;
//  }

  return TS_OPEN_OK;
}


int register_ts_thread(void) {

	int ret;

  	if (lookup.me != -1) {
		return TS_REGISTER_ERROR;
  	}

	ret = ioctl(fd, IOCTL_REGISTER_THREAD);
  	if (ret == -1) {
		return TS_REGISTER_ERROR;
  	}
	
	pid = getpid();
	lookup.me = pid;
	lookup.ts_id = ret;

  	return TS_REGISTER_OK;

}

int deregister_ts_thread(void) {
	int ret;
	if (lookup.me != getpid()){
		return TS_DEREGISTER_ERROR;
	}
	
	ret = ioctl(fd, IOCTL_DEREGISTER_THREAD, (unsigned int)lookup.ts_id);
	if (ret == -1){
		return TS_DEREGISTER_ERROR;
	}

	lookup.me = -1;
	lookup.ts_id = -1;
	return TS_DEREGISTER_OK;


}

int register_callback(void* addr) {
	int ret;

	//printf("library has been called\n");

	if (lookup.me != getpid()){
		return TS_REGISTER_CALLBACK_ERROR;
	}
	
	ret = ioctl(fd, IOCTL_SETUP_CALLBACK, addr);

	if (ret == -1){
		return TS_REGISTER_CALLBACK_ERROR;
	}


	return TS_REGISTER_CALLBACK_OK;

}

/*
int ts_start(unsigned int millisec) {

	if(lookup.me == -1){
		return TS_START_ERROR;
	}	

	buff[lookup.ts_id].millisec = millisec;
	buff[lookup.ts_id].user = 1;
	buff[lookup.ts_id].kernel = 0;

	return TS_START_OK;

}

int ts_end(void) {

	if(lookup.me == -1){
		return TS_END_ERROR;
	}	

	buff[lookup.ts_id].user = 0;
	buff[lookup.ts_id].millisec = 0;
	buff[lookup.ts_id].kernel = 0;

	return TS_END_OK;

}
*/
/*
void return_from_kernel(void) {
	printf(".");

	buff[lookup.ts_id].user++;
}
*/

