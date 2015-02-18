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
* @brief This is the header file to use the timestrech library
* @author Francesco Quaglia
* @author Alessandro Pellegrini
*
* @date November 11, 2014
*/
#pragma once
#ifndef _TIMESTRETCH_LIB
	#define _TIMESTRETCH_LIB



#define TS_OPEN_OK     1
#define TS_OPEN_ERROR -1

#define TS_REGISTER_OK     2
#define TS_REGISTER_ERROR -2

#define TS_ALREADY_OPEN -3

#define TS_DEREGISTER_OK     4
#define TS_DEREGISTER_ERROR -4

#define TS_START_OK     5
#define TS_START_ERROR -5

#define TS_END_OK    -6
#define TS_END_ERROR -6

#define TS_REGISTER_CALLBACK_OK     7
#define TS_REGISTER_CALLBACK_ERROR -7

int ts_open(void);
int register_ts_thread(void);
int deregister_ts_thread(void);
int ts_start(unsigned int millisec);
int ts_end(void);


#endif /* _TIMESTRETCH_LIB */
