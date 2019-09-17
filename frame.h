/*
* frame.h
* Copyright (c) 2019 Tsuyoshi Ohashi
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html 
*/
#ifndef _FRAME_H_
#define _FRAME_H_

#include "stdint.h"
#include <stdbool.h>

#include "enddevice.h"

typedef bool boolean;
typedef unsigned char   u1_t;
typedef unsigned short  u2_t;
typedef unsigned int    u4_t;
typedef u1_t* xref2u1_t;

#define AES_ENC       0x00 
#define AES_DEC       0x01
#define AES_MIC       0x02
#define AES_CTR       0x04
#define AES_MICNOAUX  0x08

u4_t os_aes(u1_t, xref2u1_t, u2_t);

extern u4_t AESAUX[];
extern u4_t AESKEY[];
#define AESkey ((u1_t*)AESKEY)
#define AESaux ((u1_t*)AESAUX)

extern buf_t phy_pld;

typedef struct {
    uint8_t MType;
    uint8_t Major;
    uint32_t DevAdr;
    uint8_t FCtrl;
        uint8_t ADR;        // bit7 0x80
        uint8_t ADRACKReq;  // bit6 0x40
        uint8_t ACK;        // bit5 0x20
        uint8_t ClassB;     // bit4 0x10 downlink
        uint8_t FPending;   // bit4 0x10 uplink
    uint8_t FOptLen;
    uint32_t FCnt;
    uint8_t FOpts[16];
    uint8_t FPort;
    uint8_t FRMPayload[128];
    uint32_t Mic;
    uint16_t Crc;
} frame_t;

uint32_t os_rmsbf4 (uint8_t* );
void os_wlsbf4 (uint8_t* , uint32_t );
void buildFrame(  buf_t*, uint8_t*, int);
void initAES();
void micB0 (uint32_t , uint32_t , int , int );
bool aes_verifyMic(const uint8_t* , uint32_t , uint32_t , int , uint8_t* , int );
void aes_appendMic (const uint8_t* , uint32_t , uint32_t , int , uint8_t* , int );
void aes_cipher (const uint8_t* , uint32_t , uint32_t , int , uint8_t* , int);
boolean decodeFrame(uint8_t *, int);
void frame_tx_done(void);

#endif // _FRAME_H_