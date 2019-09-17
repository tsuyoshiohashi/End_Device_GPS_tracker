/*
* timer.c
* Copyright (c) 2019 Tsuyoshi Ohashi
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html 
*/
#include <sys/timerfd.h>

#include "timer.h"

int init_timerfd(int tms, int tmn, int itm, struct itimerspec *ptime){
     int tfd;

     tfd = timerfd_create( CLOCK_MONOTONIC, TFD_NONBLOCK );
     ptime->it_value.tv_sec  = tms;
     ptime->it_value.tv_nsec = tmn;     //0;
     ptime->it_interval.tv_sec  = itm;      // do once
     ptime->it_interval.tv_nsec = 0;
     return(tfd);
}
// end of timer.c