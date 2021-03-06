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
* @file timestretch.h
* @brief This is the header file to configure the timestretch module
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @date November 11, 2014
*/
#pragma once
#ifndef __KERNEL_TIME_SLICE_STRETCH
#define __KERNEL_TIME_SLICE_STRETCH

#include <linux/ioctl.h>

#define TSTRETCH_IOCTL_MAGIC 'T'

#define UNLIKELY_FLAG 0xf0f0f0f0
#define RESET_FLAG    0x00000000

#define MAX_STRETCH 3000 //this is expressed in milliseconds

#define DELAY 1000
#define ENABLE if(1)
#define MAX_CPUs 32
#define TS_THREADS 32 //max number of concurrent HTM threads with time stretch

typedef struct _ioctl_info {
	int ds;
	void* addr;
} ioctl_info;


typedef struct _flags {
        int user;
        int kernel;
        unsigned int millisec;
} flags;

typedef struct _map {
        int me;
        int ts_id;
} map;

typedef struct _reg_info{
	int mask;
	int descriptor;
} reg_info;

#define HTM_THREADS TS_THREADS // max number of concurrent threads rnning HTM transactions with timer stretch

#define IOCTL_REGISTER_THREAD _IOW(TSTRETCH_IOCTL_MAGIC, 0, void *) 
#define IOCTL_DEREGISTER_THREAD _IO(TSTRETCH_IOCTL_MAGIC, 1) 
//#define IOCTL_SETUP_BUFFER _IOW(TSTRETCH_IOCTL_MAGIC, 2, unsigned long ) 
#define IOCTL_SETUP_CALLBACK _IOW(TSTRETCH_IOCTL_MAGIC, 3, void *) 

#endif /* __KERNEL_TIME_SLICE_STRETCH */
