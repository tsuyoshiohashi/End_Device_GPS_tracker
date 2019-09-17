/*
 *  enddevice.c     GPS version
 * Copyright (c) 2019 Tsuyoshi Ohashi
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html 
 */
/*******************************************************************************
 *
 * Copyright (c) 2015 Thomas Telkamp
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 *******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <gps.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "config.h"
#include "enddevice.h"
#include "radio.h"
#include "timer.h"
#include "frame.h"
#include "appdata.h"

extern enum sf_t sf;
extern uint32_t freq;
extern char message[256];
// valiables
uint32_t nb_rx_rcv=0;
uint32_t nb_rx_ok=0;
uint32_t nb_rx_bad=0;
uint32_t nb_tx_snd=0;

buf_t tbuf;
buf_t phy_pld;
time_t t;

// application data sample
#define BUF_S 128
uint8_t telegram[BUF_S];
//uint32_t cntr =0;

/////////
/* display error message and exit */
void die(const char *s){
    perror(s);
    exit(1);
}

// display time and event
void show_tm(void){
    struct timespec tvToday; // for msec
    struct tm *ptm; // for date and time

    clock_gettime(CLOCK_REALTIME_COARSE, &tvToday);
    ptm = localtime(&tvToday.tv_sec);
    // time
    printf("%02d:%02d:%02d.",ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    // msec
    printf("%03d ", (uint16_t)(tvToday.tv_nsec / 1000000));
}
void show_time(const char *mess){
    show_tm();
    printf(": %s\n", mess);
    fflush(stdout);
}
void show_time2(const char *mess, int val){
    show_tm();
    printf(": %s %d\n", mess, val);
    fflush(stdout);
}
void show_timeh(const char *mess, int hex_val){
    show_tm();
    printf(": %s %#x\n", mess, hex_val);
    fflush(stdout);
}
/* 
*  End Device version 
*/
int main(void){

    //// Setup ////
    const char date[] = __DATE__ ;
    const char time[] = __TIME__ ;
    int i=0;
    int rx_len=0;
    int keepalive_time = DEFAULT_KEEPALIVE; /* send a PULL_DATA request every X seconds, negative = disabled */
    int tfd_keepalive;
    uint64_t exp;
    struct itimerspec katime;
    ssize_t rtn;    // read timer status
    int tx_len=0;
    
    /* display result */
    printf("--Lora End Device -- GPS tracker ver.--\n");
    printf("Build Date: %s,%s\n", date,time);
    printf("Listening at SF%i on %.6lf Mhz.\n", sf,(double)freq/1000000);
    printf("Tx data every %d seconds.\n", keepalive_time);
    printf("----------------------------------\n");
    //// Setup LoRa Hardware ////
    wiringPiSetup () ;
    initpinmode();
    SetupwiringPiSPI();
    SetupLoRa();
    //// GPS ////
    gps_init();

    //// Setup Timers ////
    tfd_keepalive = init_timerfd(keepalive_time, 0, keepalive_time, &katime);
    timerfd_settime( tfd_keepalive, 0, &katime, NULL );

    initAES();
    ////// Watch event and do job //////
    while(1){
        ////// check RxCont mode //////
        ////// check radio packet //////
        if( readRegister(REG_OPMODE) == SX72_MODE_RX_CONTINUOS){
            rx_len = rf_receivepacket();
            if(rx_len > 1 ){
                decodeFrame((uint8_t*)message, rx_len);
                ////// received app data is set  //////
                ////// in phy_pld                //////
                if(phy_pld.bytes >0 ){
                    printf("\tReceived Data: ");
                    for( i=0;i<phy_pld.bytes; i++){
                        printf("%#x:",phy_pld.buf[i]);
                    }
                    printf("\n");              
                }
            }
        }   
        ////// Check STDBY(TxDone) and Resume RxCont //////
        if( readRegister(REG_OPMODE) == SX72_MODE_STDBY){
            show_time("Txdone, Restart RX_CONT.");
            // clear radio IRQ flags
            writeRegister(REG_IRQ_FLAGS, 0xFF);
            //mask all radio IRQ flags but RxDone 
            writeRegister(REG_IRQ_FLAGS_MASK,  ~SX1276_IRQ_RXDONE_MASK);
            // Set DIO0 RxDone 
            writeRegister(REG_DIO_MAPPING_1, 0x00);
            // Set DevMode TXDone(STDBY) -> RX_Cont
            writeRegister(REG_OPMODE, SX72_MODE_RX_CONTINUOS);
            frame_tx_done();
        }
        ////// check timer keepalive //////            
        rtn = read( tfd_keepalive, &exp, sizeof(uint64_t) );
        if ( rtn < 0){
            if( errno != EINTR && errno != EAGAIN){
                die("tfd_keepalive");
            }
        }else{
            show_time("Keepalive timer expired");
            show_time( "Transmit rf pkt.");
            // build frame
            ////// set tx app data to telegram buffer UP TO 115char /////
            tx_len = set_app_data( (uint8_t*)telegram);
            //printf("\tApp Data:");      
            //for( int i=0;i< tx_len; i++){
            //    printf("%02x:",telegram[i]);
            //}
            //printf("\n");      
            
            buildFrame( &tbuf, (uint8_t*) telegram, tx_len);
            rf_transmitpacket(&tbuf);
        }
    }   // end of while
    return(0);     
}
// end of enddevice.c
