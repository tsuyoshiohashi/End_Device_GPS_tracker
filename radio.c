/*  radio.c
* Copyright (c) 2019 Tsuyoshi Ohashi
* All rights reserved. This program and the accompanying materials
* are made available under the terms of the Eclipse Public License v1.0
* which accompanies this distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html 
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "enddevice.h"
#include "config.h"
#include "radio.h"
#include "frame.h"

// SX1272 - Raspberry connections
int ssPin = 6;
int dio0  = 7;
int RST   = 0;
//int rxtxsw = ;

//raspi SPI Channel
static const int CHANNEL = 0;

// variables
char message[256];

bool sx1272 = true;

// debug
int debug = 0;
// Set spreading factor (SF7 - SF12)
enum sf_t sf= SF7;

// Set center frequency
//uint32_t  freq = 868100000; // in Mhz! (868.1)
uint32_t freq = FREQUENCY;
uint32_t freq2 = FREQ_RX2;

//	Power Setting for Japan ARIB STD-T108 
//  20mW=13dBm
////----UPDATED----SELECT PA_BOOST----////
//  SX1276 RegPaConfig(0x09)    Val=1.011.1011=0xbb
//  bit 7 	PaSelect = 1		select PA_BOOST (not RFO!)
//  bit 6-4	MaxPower = 3		Pmax=10.8+0.6*3=12.6 < 13
//  bit 3-0 OutputPower = 11 =b	Pout=17-(15-OutputPower)=13
int  PWR_JPN_1276 = 0xbb;       //3f;

// SX1272 RegPaCfg(0x09)        Val=1.000.1011=0x8b
//  bit 7 	PaSelect = 1		select PA_BOOST (not RFO!)
//  bit 6-4	n/a = 0     		Pmax=2+OutputPower = 13
//  bit 3-0 OutputPower = 11 = b	 
int  PWR_JPN_1272 = 0x8b;

extern buf_t tbuf;
extern uint32_t nb_rx_rcv;
extern uint32_t nb_rx_ok;
extern uint32_t nb_rx_bad;
extern uint32_t nb_rx_nocrc;
extern uint32_t nb_tx_snd;

byte receivedbytes;
int buff_index;

void SetupwiringPiSPI(){
    wiringPiSPISetup(CHANNEL, 500000);
}
void initpinmode(void){
    pinMode(ssPin, OUTPUT);
    pinMode(dio0, INPUT);
    pinMode(RST, OUTPUT);
    // antenna switch
    //pinMode(255, OUTPUT);
}
//
void selectreceiver(void){
    digitalWrite(ssPin, LOW);
}

void unselectreceiver(void){
    digitalWrite(ssPin, HIGH);
}

byte readRegister(byte addr){
    unsigned char spibuf[2];

    selectreceiver();
    spibuf[0] = addr & 0x7F;
    spibuf[1] = 0x00;
    wiringPiSPIDataRW(CHANNEL, spibuf, 2);
    unselectreceiver();

    return spibuf[1];
}

void writeRegister(byte addr, byte value){
    unsigned char spibuf[2];

    spibuf[0] = addr | 0x80;
    spibuf[1] = value;
    selectreceiver();
    wiringPiSPIDataRW(CHANNEL, spibuf, 2);

    unselectreceiver();
}

void set_Opmode (byte opmode) {
    
    writeRegister(REG_OPMODE, opmode);
}

void cfg_Modem(void){
    // 8 Symbols 
    writeRegister(REG_PREAMBLE_LENGTH_LSB, 0x08);
    // datar
    // code rate
    // crc
    // ipol
    uint8_t mc1=0, mc2=0, mc3=0;
    mc1 |= 0x70;    //SX1276_MC1_BW_125;
    mc1 |= 0x02;    //SX1276_MC1_CR_4_5;
    writeRegister(REG_MODEM_CONFIG, mc1);

    mc2 |= 0x70;    //(SX1272_MC2_SF7 + ((sf-1)<<4));
    mc2 |= 0x04;    //SX1276_MC2_RX_PAYLOAD_CRCON;
    writeRegister(REG_MODEM_CONFIG2, mc2);
        
    mc3 |= SX1276_MC3_AGCAUTO;
    writeRegister(REG_MODEM_CONFIG3, mc3);    
    // ipol
    writeRegister(REG_INV_IQ, readRegister(REG_INV_IQ)| SX1276_INV_IQ_TX);
}
//
void set_Channel(uint32_t frq){
    
    uint64_t frf = ((uint64_t)frq << 19) / 32000000;
    writeRegister(REG_FRF_MSB, (uint8_t)(frf>>16) );
    writeRegister(REG_FRF_MID, (uint8_t)(frf>> 8) );
    writeRegister(REG_FRF_LSB, (uint8_t)(frf>> 0) );
}
// Set Ch for RX2
void set_Ch2(void){
    set_Channel(freq2);
}

void set_DataRate(enum sf_t sf){
    if (sx1272) {
        if (sf == SF11 || sf == SF12) {
            writeRegister(REG_MODEM_CONFIG,0x0B);
        } else {
            writeRegister(REG_MODEM_CONFIG,0x0A);
        }
        writeRegister(REG_MODEM_CONFIG2,(sf<<4) | 0x04);
    } else {
        if (sf == SF11 || sf == SF12) {
            writeRegister(REG_MODEM_CONFIG3,0x0C);
        } else {
            writeRegister(REG_MODEM_CONFIG3,0x04);
        }
        writeRegister(REG_MODEM_CONFIG,0x72);
        writeRegister(REG_MODEM_CONFIG2,(sf<<4) | 0x04);
    }

    if (sf == SF10 || sf == SF11 || sf == SF12) {
        writeRegister(REG_SYMB_TIMEOUT_LSB,0x05);
    } else {
        writeRegister(REG_SYMB_TIMEOUT_LSB,0x08);
    }
}

void SetupLoRa(void){
    
    digitalWrite(RST, HIGH);
    delay(100);
    digitalWrite(RST, LOW);
    delay(100);

    byte version = readRegister(REG_VERSION);

    if (version == 0x22) {
        // sx1272
        //printf("SX1272 detected, starting.\n");
        show_time("SX1272 detected, starting.");
        sx1272 = true;
    } else {
        // sx1276?
        digitalWrite(RST, LOW);
        delay(100);
        digitalWrite(RST, HIGH);
        delay(100);
        version = readRegister(REG_VERSION);
        if (version == 0x12) {
            // sx1276
            //printf("SX1276 detected, starting.\n");
            show_time("SX1276 detected, starting.");
            sx1272 = false;
        } else {
            printf("Unrecognized transceiver.\n");
            //printf("Version: 0x%x\n",version);
            exit(1);
        }
    }
    // Lora+Sleep
    writeRegister(REG_OPMODE, SX72_MODE_SLEEP);

    // set frequency
    set_Channel(freq);
    
    //// Set Tx Power for Japan ////
    if (sx1272) {
		writeRegister(REG_PA_CFG, PWR_JPN_1272);
	} else {
		// sx1276
        // configure output power
        writeRegister(REG_PA_RAMP, 0x08); // set PA ramp-up time 50 uSec
		writeRegister(REG_PA_CFG, PWR_JPN_1276);
	}
/////////////////////////////////////////////////////////////
    writeRegister(REG_SYNC_WORD, 0x34); // LoRaWAN public sync word
    
    set_DataRate(sf);
    //
    writeRegister(REG_MAX_PAYLOAD_LENGTH,0x80);
    writeRegister(REG_PAYLOAD_LENGTH,PAYLOAD_LENGTH);
    writeRegister(REG_HOP_PERIOD,0xFF);
    writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_BASE_AD));

    // Set Continous Receive Mode
    writeRegister(REG_LNA, LNA_MAX_GAIN);  // max lna gain
    // Start Rx     0x80=Lora, 0x05=RX_CONT
    writeRegister(REG_OPMODE, SX72_MODE_RX_CONTINUOS);

}

boolean receivePkt(char *payload){

    // clear rxDone
    writeRegister(REG_IRQ_FLAGS, 0x40);

    int irqflags = readRegister(REG_IRQ_FLAGS);
    nb_rx_rcv++;
    //  payload crc: 0x20
    if((irqflags & 0x20) == 0x20){
        printf("CRC error\n");
        writeRegister(REG_IRQ_FLAGS, 0x20);
        nb_rx_bad++;
        return false;
    } else {
        nb_rx_ok++;
        show_time("rx radio packet");
        byte currentAddr = readRegister(REG_FIFO_RX_CURRENT_ADDR);
        byte receivedCount = readRegister(REG_RX_NB_BYTES);
        receivedbytes = receivedCount;
        writeRegister(REG_FIFO_ADDR_PTR, currentAddr);

        printf("\tSX FIFO DATA: ");
        for(int i = 0; i < receivedCount; i++){
            //char?
            payload[i] = (char)readRegister(REG_FIFO);
            printf("%#x:", payload[i]);
        }
        printf("\n");
    }
    return true;
}

int rf_receivepacket(void) {

    long int SNR;
    int rssicorr;
    int rssi;
    //int buff_index = 0;
    //int j;
    //DIO0: Map=00:RxDone
    //      Map=01:TxDone
    if(digitalRead(dio0) == 1){
        if(receivePkt(message)) {
            byte value = readRegister(REG_PKT_SNR_VALUE);
            if( value & 0x80 ){ // The SNR sign bit is 1
                // Invert and divide by 4
                value = ( ( ~value + 1 ) & 0xFF ) >> 2;
                SNR = -value;
            }else{
                // Divide by 4
                SNR = ( value & 0xFF ) >> 2;
            }
            
            if (sx1272) {
                rssicorr = 139;
            } else {
                rssicorr = 157;
            }
            rssi = readRegister(0x1A)-rssicorr;

            printf("\tPacket RSSI: %d, ", rssi);
            printf("RSSI: %d, ",readRegister(0x1B)-rssicorr);
            printf("SNR: %li, ",SNR);
            printf("Length: %i",(int)receivedbytes);
            printf("\n");
            return(receivedbytes);
        }else{
            return(-1);
        } // received a message
    }else{
        return(-1);
    } // dio0=1
}

void transmitPkt(buf_t* tbuf){
    uint8_t i=0;

    //assert( readRegister(REG_OPMODE) == SX72_MODE_RX_CONTINUOS);
    // Stop Rx, enter standby mode (required for FIFO loading))
    //writeRegister(REG_OPMODE, SX72_MODE_STDBY);

    set_Opmode(SX72_MODE_STDBY);
    //assert( readRegister(REG_OPMODE) == SX72_MODE_STDBY);
    // configure LoRa modem (cfg1, cfg2,cfg3)
    cfg_Modem();
    
    set_Channel(freq);

    // set sync word already done
    writeRegister(REG_SYNC_WORD, LORA_MAC_PREAMBLE);

    // set the IRQ mapping DIO0=TxDone DIO1=NOP DIO2=NOP
    // bit7-6:DIO0 00=RxDone,01=TxDone,10=CadDone
    writeRegister(REG_DIO_MAPPING_1, 0x40);
    
    // clear all radio IRQ flags
    writeRegister(REG_IRQ_FLAGS, 0xFF);
    // mask all IRQs but TxDone
    writeRegister(REG_IRQ_FLAGS_MASK, ~SX1276_IRQ_TXDONE_MASK);

    // initialize the payload size and address pointers    
    writeRegister(REG_FIFO_TX_BASE_AD, 0x00);
    writeRegister(REG_FIFO_ADDR_PTR, 0x00);
    //writeRegister(REG_FIFO_ADDR_PTR, REG_FIFO_TX_BASE_AD);
    writeRegister(REG_PAYLOAD_LENGTH, tbuf->bytes);
    // download buffer to the radio FIFO
    for( i = 0; i < tbuf->bytes; i++){
        writeRegister(REG_FIFO,tbuf->buf[i]);
    }
   
    // DEBUG
    //for(i=1; i< 0x42; i++){
    //    printf(" REG%#x: %#x\n",i, readRegister(i));
    //}
    // now we actually start the transmission
    set_Opmode(SX72_MODE_TX);
    // Transition FSTX -> TX in chip
    //show_timeh("OPMODE=",readRegister(REG_OPMODE));
    //assert( readRegister(REG_OPMODE) == SX72_MODE_TX);
}

void rf_transmitpacket(buf_t* tbuf_p){
    int i=0;
    printf("\t%d bytes PHYPayload to tx. ", tbuf_p->bytes);
    for(i=0; i< tbuf_p->bytes; i++){
        printf("%#x:",tbuf_p->buf[i]);
     }
     printf("\n");
    transmitPkt(tbuf_p);
    nb_tx_snd++;
}

boolean is_channel_free(){
    int rssi;
    rssi = (int)readRegister(REG_RSSI_VAL);
    rssi -= RSSI_CORR;
    if(rssi <= RSSI_CH_FREE){
        show_time2("Channel is Free. ", rssi);
        return(TRUE);
    }else{
        show_time2("Channel is Busy. ", rssi);
        return(FALSE);    
    }
}
// end of radio.c
