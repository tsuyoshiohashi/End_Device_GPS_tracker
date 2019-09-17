/*  enddevice.h
* Copyright (c) 2019 Tsuyoshi Ohashi
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html 
*/

#ifndef _ENDDEVICE_H_
#define _ENDDEVICE_H_

#define BUF_SIZE 1024
// typedef
typedef bool boolean;
typedef unsigned char byte;

typedef struct {
     int bytes;
     uint8_t buf[BUF_SIZE];
} buf_t;

// proto type declaration 
void die( const char *);
void show_tm(void);
void show_time(const char *);
void show_time2(const char *, int);
void show_timeh(const char *, int);

#endif // _ENDDEVICE_H_