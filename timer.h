/*
* timer.h
* Copyright (c) 2019 Tsuyoshi Ohashi
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html 
*/
#ifndef _TIMER_H_
#define _TIMER_H_

// sec, nanosec, interval sec, timerspec
int init_timerfd(int, int, int, struct itimerspec*);

#endif // _TIMER_H_