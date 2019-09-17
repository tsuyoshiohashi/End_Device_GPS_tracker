/*
* frame.c
* Copyright (c) 2019 Tsuyoshi Ohashi
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html 
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "enddevice.h"
#include "frame.h"

frame_t rxFrm;
frame_t txFrm;

uint8_t nwkKey[16];
uint8_t artKey[16];
uint8_t devKey[16];

uint32_t FCntUp = 0;
uint32_t NFCntDown =0;
uint32_t AFCntDown =0;

uint32_t os_rmsbf4 (uint8_t* buf) {
    return (uint32_t)(buf[3] | (buf[2]<<8) | ((uint32_t)buf[1]<<16) | ((uint32_t)buf[0]<<24));
}

void os_wlsbf4 (uint8_t* buf, uint32_t v) {
    buf[0] = v;
    buf[1] = v>>8;
    buf[2] = v>>16;
    buf[3] = v>>24;
}
void os_wmsbf4 (uint8_t* buf, uint32_t v) {
    buf[3] = v;
    buf[2] = v>>8;
    buf[1] = v>>16;
    buf[0] = v>>24;
}

void buildFrame(  buf_t* tbuf_p, uint8_t* tx_mes, int tx_mlen){

    txFrm.MType = 0x02;     // 0x02=Unconfirmed Data Up
    txFrm.Major = 0x00;     // 0x00=LoRaWANR1
    txFrm.DevAdr = DEVADDR;
    txFrm.FCtrl = txFrm.ACK;   // ADR | ADRACKReq | ACK | classB | FOptLen
    FCntUp +=1;
    txFrm.FCnt = (FCntUp) & 0xffff;  // 
    txFrm.FPort = 1;        // 0=MAC command, 1-223=application specific, 224=test

    tbuf_p->buf[0] = txFrm.MType << 5 | txFrm.Major;
    os_wlsbf4(tbuf_p->buf+1,  txFrm.DevAdr);
    tbuf_p->buf[5] = txFrm.FCtrl;
    tbuf_p->buf[6] = txFrm.FCnt;
    tbuf_p->buf[7] = txFrm.FCnt>>8;
    // Let FOpts=0
    tbuf_p->buf[8] = txFrm.FPort;
    for(int i=0; i<tx_mlen; i++){
        tbuf_p->buf[i+9] = tx_mes[i];
    }
    tbuf_p->buf[tx_mlen+9] = '\0';       // this version is for char (not binary)
   
    aes_cipher( txFrm.FPort==0 ? DEVKEY : ARTKEY, txFrm.DevAdr, txFrm.FCnt, /*up*/0, &tbuf_p->buf[9], tx_mlen);
    aes_appendMic(DEVKEY, txFrm.DevAdr, txFrm.FCnt, /*up*/0, tbuf_p->buf, tx_mlen+9);
    printf("\tMT:%#x,Mj:%x,DA:%#x,FCntrl:%#x,FCt:%d,FOL:%d,FP:%d\n",\
        txFrm.MType,txFrm.Major,txFrm.DevAdr,txFrm.FCtrl,txFrm.FCnt,txFrm.FOptLen,txFrm.FPort);
    
    tbuf_p->bytes =  tx_mlen+9+4;
    // crc?
}

void initAES(){
    //
    memcpy(AESkey,DEVKEY,16);
}

void micB0 (uint32_t devaddr, uint32_t seqno, int dndir, int len) {
    memset(AESaux,0x00, 16);
    AESaux[0]  = 0x49;
    AESaux[5]  = dndir?1:0;
    AESaux[15] = len;
    os_wlsbf4(AESaux+ 6,devaddr);
    os_wlsbf4(AESaux+10,seqno);
}

boolean aes_verifyMic(const uint8_t* key, uint32_t devaddr, uint32_t seqno, int dndir, uint8_t* payload, int len) {
    micB0(devaddr, seqno, dndir, len);
    memcpy(AESkey,key,16);
    uint32_t mic_c = os_aes(AES_MIC, payload, len);
    //uint32_t mic_f = rxFrm.Mic;         //os_rmsbf4(payload+len);
    printf("\tcal_Mic:%#x, frm_Mic:%#x\n", mic_c, rxFrm.Mic);  //os_rmsbf4(payload+len) );
    if(mic_c == rxFrm.Mic)
        return(1);
    else
        return(0);        
}

 void aes_appendMic (const uint8_t* key, uint32_t devaddr, uint32_t seqno, int dndir, uint8_t* payload, int len) {
    micB0(devaddr, seqno, dndir, len);
    memcpy(AESkey,key,16);
    // MSB because of internal structure of AES
    os_wmsbf4(payload+len, os_aes(AES_MIC, payload, len));
}

void aes_cipher (const uint8_t* key, uint32_t devaddr, uint32_t seqno, int dndir, uint8_t* payload, int len) {
    if( len <= 0 )
        return;
    //printf("\tEnc:");
    //for(int i=0;i<len; i++){
    //    printf("%#x:",payload[i]);
    //}
    //printf("\n");
    memset(AESaux, 0x00, 16);
    AESaux[0] = AESaux[15] = 1; // mode=cipher / dir=down / block counter=1
    AESaux[5] = dndir?1:0;
    os_wlsbf4(AESaux+ 6,devaddr);
    os_wlsbf4(AESaux+10,seqno);
    memcpy(AESkey,key,16);
    os_aes(AES_CTR, payload, len);
    //printf("\tCph:");
    //for( int i=0;i<len; i++){
    //    printf("%#x:",payload[i]);
    //}
    //printf("\n");
}

boolean decodeFrame(uint8_t * mes, int mlen){

    rxFrm.MType = (mes[0] & 0xe0) >> 5; //bit7-5
    rxFrm.Major = mes[0] & 0x03;
    rxFrm.DevAdr = ( mes[4] << 24 | mes[3] << 16 | mes[2] << 8 | mes[1] );
    rxFrm.FCtrl = mes[5];
    // Frame Control
    rxFrm.ACK = rxFrm.FCtrl && 0x20; // bit5

    rxFrm.FOptLen = mes[5] & 0x0f;
    rxFrm.FCnt = (uint32_t)( mes[7] << 8 | mes[6] );
    if(rxFrm.FOptLen > 0){     // FOpts present
        memcpy(rxFrm.FOpts, &mes[8], rxFrm.FOptLen);
    }
    rxFrm.FPort = mes[8+rxFrm.FOptLen];     // 0:MAC command only, 1-223(0xDF): application specific, 224:test
    memcpy(rxFrm.FRMPayload, &mes[9 + rxFrm.FOptLen], mlen - 9 - rxFrm.FOptLen - 4);

    rxFrm.Mic =  ( mes[mlen-4] << 24 | mes[mlen-3] << 16 | mes[mlen-2] << 8 | mes[mlen-1] );
    rxFrm.Crc = 0;  // not use crc in down link
    printf("\tLen:%d,MT:%#x,Mj:%x,DA:%#x,FCntrl:%#x,FCt:%d,FOL:%d,FP:%d,MIC:%#x,NFCD:%d,AFCD:%d\n",\
        mlen,rxFrm.MType,rxFrm.Major,rxFrm.DevAdr,rxFrm.FCtrl,rxFrm.FCnt,rxFrm.FOptLen,rxFrm.FPort,rxFrm.Mic,NFCntDown,AFCntDown);    
    printf("\tFRMPayload: ");
    for(int i=0; i< (mlen-13-rxFrm.FOptLen); i++){
        printf("%#x:",rxFrm.FRMPayload[i]);
    }
    printf("\n");

    if(rxFrm.DevAdr != DEVADDR){
        show_timeh("To Another Node,%#x",rxFrm.DevAdr);
        return(0);
    }

    if( aes_verifyMic( DEVKEY, rxFrm.DevAdr, rxFrm.FCnt, 0x01, mes , mlen-4) == 0){
        show_time("MIC Error.");
        return(0);
    };
    // we get data! 
    phy_pld.bytes = mlen -13-rxFrm.FOptLen;
    uint8_t* phypld_p = mes+9+rxFrm.FOptLen;
    aes_cipher(rxFrm.FPort == 0 ? DEVKEY : ARTKEY, rxFrm.DevAdr, rxFrm.FCnt, /*dn*/1, phypld_p, phy_pld.bytes); //mlen-13-rxFrm.FOptLen);  //-4-8-1-rxFrm.FOptLen);
    memcpy(phy_pld.buf, phypld_p, phy_pld.bytes); //  
    // Set ACK return frame   
    if(rxFrm.MType==0x05){  //0x05=Confirmed Data Down
        txFrm.ACK = 0x20;   // bit5
    }else{
        txFrm.ACK = 0;
    }

    show_time2("Data Received. byte(s):",phy_pld.bytes);
     if(rxFrm.FPort == 0){
        NFCntDown +=1;
    }else{
        AFCntDown +=1;
    }
    return(1);
}

void frame_tx_done(){
    txFrm.ACK = 0;  // reset ACK
}
// end of frame.c